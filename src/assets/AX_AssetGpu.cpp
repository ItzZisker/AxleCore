#include "axle/assets/AX_AssetGpu.hpp"

// struct AssetGPUMeshBuffers {
//     gfx::BufferHandle vertices;
//     gfx::BufferHandle indices;
// };

// struct AssetGPUMaterialResources {
//     utils::CowSpan<gfx::Binding> bindings;
//     utils::Range<uint32_t> bindingsRange;

//     gfx::ResourceSetHandle resourcesHandle;
// };

// class AssetGPU {
// private:
//     SharedPtr<core::ThreadContextGfx> m_GfxThread;

//     std::unordered_map<gfx::BufferHandle, gfx::BufferDesc> m_BufferDescs;
// public:
//     explicit AssetGPU(SharedPtr<core::ThreadContextGfx> gfxThread);
//     ~AssetGPU();

//     utils::ExResult<AssetGPUMeshBuffers> UploadBuffers(const AssetMesh& mesh);
//     utils::ExResult<AssetGPUMaterialResources> UploadResources(const AssetMaterial& mat);
    
//     utils::ExResult<gfx::BufferDesc> DescribeBuffer(const gfx::BufferHandle&);
// };

using namespace axle::utils;

namespace axle::assets
{

AssetGpu::AssetGpu(SharedPtr<core::ThreadContextGfx> gfxThread)
    : m_GfxThread(gfxThread) {}

AssetGpu::~AssetGpu() {
    auto lambdaFinalizer = [
        bufferLookup = std::move(m_BufferDescs),
        textureLookup = std::move(m_TextureDescs),
        resourcesLookup = std::move(m_ResourcesDescs),
        gfxThread = m_GfxThread
    ]() {
        auto backend = gfxThread->GetContext();
        for (auto& [h, _] : resourcesLookup) backend->DestroyResourceSet(h);
        for (auto& [h, _] : textureLookup) backend->DestroyTexture(h);
        for (auto& [h, _] : bufferLookup) backend->DestroyBuffer(h);
    };
    core::InstaTaskOrQueue(*m_GfxThread.get(), lambdaFinalizer);
}

Future<ExResult<AssetGpuMeshes>> AssetGpu::UploadMeshes(AssetMeshesUploadDesc& desc) {
    auto lambdaUploader = [&, desc]() -> ExResult<AssetGpuMeshes> {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto gbgfx = m_GfxThread->GetContext();

        std::vector<ExError> errors;
        std::vector<AssetGpuMesh> meshes;

        gfx::BufferDesc verticesDesc;
        verticesDesc.access = gfx::BufferAccess::Immutable;
        verticesDesc.usage = gfx::BufferUsage::Vertex;
        verticesDesc.cpuVisible = false;

        gfx::BufferDesc indicesDesc;
        indicesDesc.access = gfx::BufferAccess::Immutable;
        indicesDesc.usage = gfx::BufferUsage::Index;
        indicesDesc.cpuVisible = false;

        for (uint32_t i{0}; i < desc.immutableMeshes.size(); i++) {
            auto& immutableMeshRef = desc.immutableMeshes[i];

            gfx::BufferHandle vertexBuffer{UINT32_MAX, UINT32_MAX};
            gfx::BufferHandle indexBuffer{UINT32_MAX, UINT32_MAX};
            
            gfx::MeshVertexLayout layout;

            auto& vertices = desc.immutableImport.buffers[immutableMeshRef.vertexBufferIdx];
            verticesDesc.size = vertices.raw.size();

            auto vres = gbgfx->CreateBuffer(verticesDesc);
            if (!vres.has_value()) {
                auto& err = vres.error();
                errors.push_back(ExError{err.GetCode(), 
                    "Vertices Upload failure at desc.meshes, i=" 
                    + std::to_string(i) + ", Error=" + std::string(err.GetMessage())
                });
            } else {
                vertexBuffer = vres.value();
                m_BufferDescs[vertexBuffer] = verticesDesc;
            }

            bool indexed = immutableMeshRef.indexBufferIdx != UINT32_MAX;

            if (indexed) {
                auto& indices = desc.immutableImport.buffers[immutableMeshRef.indexBufferIdx];
                indicesDesc.size = indices.raw.size();

                auto ires = gbgfx->CreateBuffer(indicesDesc);
                if (!ires.has_value()) {
                    auto& err = ires.error();
                    errors.push_back(ExError{err.GetCode(), 
                        "Indices Upload failure at desc.meshes, i=" 
                        + std::to_string(i) + ", Error=" + std::string(err.GetMessage())
                    });
                } else {
                    indexBuffer = ires.value();
                    m_BufferDescs[indexBuffer] = indicesDesc;
                }
            }

            auto& vDesc = GetVertexFormatDesc(immutableMeshRef.vertexFormat);

            gfx::VertexTypeDesc typeDesc {
                gfx::VertexAttributeClass::Float, 
                gfx::VertexAttributeType::Float32
            };

            uint32_t currentOffset = 0;
            const uint32_t FLOAT_SIZE = sizeof(float); // 4 bytes

            // Position (float3, POSITION0)
            std::vector<gfx::MeshVertexAttribute> attributes;
            attributes.push_back({gfx::VertexSemantic::Position, 0, 3, 0, currentOffset, FLOAT_SIZE * 3, typeDesc, false});
            currentOffset += FLOAT_SIZE * 3;
            // Normal (float3, NORMAL0)
            attributes.push_back({gfx::VertexSemantic::Normal, 0, 3, 0, currentOffset, FLOAT_SIZE * 3, typeDesc, false});
            currentOffset += FLOAT_SIZE * 3;
            // TexCoords (float2 * uv_count, TEXCOORD[0-7])
            for (uint32_t uv_i{0}; uv_i < vDesc.uvCount; uv_i++) {
                attributes.push_back({gfx::VertexSemantic::TexCoord, uv_i, 2, 0, currentOffset, FLOAT_SIZE * 2, typeDesc, false});
                currentOffset += FLOAT_SIZE * 2;
            }
            // Tangent (float3, TANGENT0) (if supported by mesh)
            if (vDesc.hasTangents) {
                attributes.push_back({gfx::VertexSemantic::Tangent, 0, 3, 0, currentOffset, FLOAT_SIZE * 3, typeDesc, false});
                currentOffset += FLOAT_SIZE * 3;
            }

            layout.attributes = utils::CowSpan{std::move(attributes)};
            layout.stride = currentOffset; 

            if (vertices.stride != currentOffset) {
                errors.push_back({"Vertices stride mismatches with currentOffset, at desc.meshes, i=" + std::to_string(i)});
            }

            AssetGpuMesh gpuMesh;
            gpuMesh.vertices = vertexBuffer;
            gpuMesh.indices = indexBuffer;
            gpuMesh.layout = layout;
            gpuMesh.indexed = indexed;

            meshes.push_back(gpuMesh);
        }

        if (!errors.empty()) {
            for (auto& mesh : meshes) {
                m_BufferDescs.erase(mesh.vertices);
                if (mesh.indexed) m_BufferDescs.erase(mesh.indices);

                gbgfx->DestroyBuffer(mesh.vertices);
                if (mesh.indexed) gbgfx->DestroyBuffer(mesh.indices);
            }
            std::stringstream error_str_stream;
            for (auto& error : errors) {
                error_str_stream << "ErrCode=" << error.GetCode() << ", "
                    << error.GetMessage() << "\n";
            }
            return ExError{error_str_stream.str()};
        }

        return AssetGpuMeshes{{std::move(meshes)}};
    };
    return core::InstaFutureOrQueue(*m_GfxThread.get(), lambdaUploader);
}

Future<ExResult<AssetGpuMaterials>> AssetGpu::UploadMaterials(AssetMaterialsUploadDesc& desc) {
    auto lambdaUploader = [&, desc]() -> ExResult<AssetGpuMaterials> {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto gbgfx = m_GfxThread->GetContext();

        std::vector<AssetGpuMaterial> materials;
        std::vector<ExError> errors;
        
        for (uint32_t i{0}; i < desc.immutableMaterials.size(); i++) {
            auto& argMat = desc.immutableMaterials[i];
            auto& mat = argMat.immutableMaterial;

            gfx::ResourceSetDesc rDesc;
            rDesc.setIndex = argMat.resourceSetIndex;

            if (!mat.imported) {
                errors.push_back({"Material at index=" + std::to_string(i) + " is not yet imported!"});
                continue;
            }

            for (uint32_t i{0}; i < mat.texture_indices.size(); i++) {
                auto texType = (MaterialTextureType) i;
                auto& texIndices = mat.texture_indices[i];

                
            }
        }
        
        if (!errors.empty()) {
            for (auto& material : materials) {
                m_ResourcesDescs.erase(material.resourcesHandle);
                gbgfx->DestroyResourceSet(material.resourcesHandle);
            }
            std::stringstream error_str_stream;
            for (auto& error : errors) {
                error_str_stream << "ErrCode=" << error.GetCode() << ", "
                    << error.GetMessage() << "\n";
            }
            return ExError{error_str_stream.str()};
        }

        return AssetGpuMaterials{{std::move(materials)}};
    };
    return core::InstaFutureOrQueue(*m_GfxThread.get(), lambdaUploader);
}

ExResult<gfx::ResourceSetDesc> AssetGpu::DescribeBindings(const gfx::ResourceSetHandle& handle) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto found = m_ResourcesDescs.find(handle);
    if (found == m_ResourcesDescs.end()) return ExError{"Invalid ResourceSetHandle"};
    return found->second;
}

ExResult<gfx::TextureDesc> AssetGpu::DescribeTexture(const gfx::TextureHandle& handle) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto found = m_TextureDescs.find(handle);
    if (found == m_TextureDescs.end()) return ExError{"Invalid TextureHandle"};
    return found->second;
}

ExResult<gfx::BufferDesc> AssetGpu::DescribeBuffer(const gfx::BufferHandle& handle) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto found = m_BufferDescs.find(handle);
    if (found == m_BufferDescs.end()) return ExError{"Invalid BufferHandle"};
    return found->second;
}

}