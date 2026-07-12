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

AssetGpu::AssetGpu(ThreadGfxScope gfxThread)
    : ThreadOwned(gfxThread) {}

AssetGpu::~AssetGpu() {
    ThreadInvocationVoid(m_Thread, [&](){
        auto gbgfx = m_Thread->GetContext();

        for (auto& h : m_HeldBuffers) gbgfx->DestroyBuffer(h);
        for (auto& h : m_HeldTextures) gbgfx->DestroyTexture(h);
        for (auto& h : m_HeldResources) gbgfx->DestroyResourceSet(h);

        return VoidInvoke{};
    }).SyncCall();
}

ThreadInvocation<ExResult<AssetGpuMeshes>> AssetGpu::UploadMeshes(AssetMeshesUploadDesc& desc) {
    ThreadInvocation<ExResult<AssetGpuMeshes>>(m_Thread, [&]() -> ExResult<AssetGpuMeshes> {
        auto gbgfx = m_Thread->GetContext();

        std::vector<ExError> errors;
        std::vector<AssetGpuMesh> meshes;

        std::vector<gfx::BufferHandle> localHeldBuffers;

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
                localHeldBuffers.push_back(vertexBuffer);
            }

            bool indexed = immutableMeshRef.indexBufferIdx != UINT32_MAX;
            uint32_t indexCount{0};

            if (indexed) {
                auto& indices = desc.immutableImport.buffers[immutableMeshRef.indexBufferIdx];
                indicesDesc.size = indices.raw.size();
                indexCount = indices.count;

                auto ires = gbgfx->CreateBuffer(indicesDesc);
                if (!ires.has_value()) {
                    auto& err = ires.error();
                    errors.push_back(ExError{err.GetCode(), 
                        "Indices Upload failure at desc.meshes, i=" 
                        + std::to_string(i) + ", Error=" + std::string(err.GetMessage())
                    });
                } else {
                    indexBuffer = ires.value();
                    localHeldBuffers.push_back(indexBuffer);
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
            gpuMesh.vertexCount = vertices.count;
            gpuMesh.indexCount = indexCount;
            gpuMesh.indexed = indexed;

            meshes.push_back(gpuMesh);
        }

        if (!errors.empty()) {
            for (auto& mesh : meshes) {
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

        m_HeldBuffers.insert(
            m_HeldBuffers.end(),
            std::make_move_iterator(localHeldBuffers.begin()),
            std::make_move_iterator(localHeldBuffers.end())
        );

        return AssetGpuMeshes{{std::move(meshes)}};
    });
}

ThreadInvocation<ExResult<AssetGpuMaterials>> AssetGpu::UploadMaterials(AssetMaterialsUploadDesc& desc) {
    return ThreadInvocation<ExResult<AssetGpuMaterials>>(m_Thread, [&, desc]() -> ExResult<AssetGpuMaterials> {
        auto gbgfx = m_Thread->GetContext();

        std::vector<AssetGpuMaterial> materials;
        std::vector<ExError> errors;

        std::vector<gfx::BufferHandle> localHeldBuffers;
        std::vector<gfx::TextureHandle> localHeldTextures;
        std::vector<gfx::ResourceSetHandle> localHeldResources;
        
        for (uint32_t matIdx{0}; matIdx < desc.immutableMaterials.size(); matIdx++) {
            auto& argMat = desc.immutableMaterials[matIdx];
            auto& mat = argMat.immutableMaterial;

            std::vector<gfx::Binding> bindings;
            std::vector<gfx::TextureHandle> textureHandles;
            gfx::BufferHandle propsUbo;

            if (!mat.imported) {
                errors.push_back({"Material at matIdx=" + std::to_string(matIdx) + " is not yet imported!"});
                continue;
            }

            constexpr std::size_t MAT_PROPS_SIZE = sizeof(MaterialProps);

            gfx::BufferDesc propsUboDesc;
            propsUboDesc.size = MAT_PROPS_SIZE;
            propsUboDesc.usage = gfx::BufferUsage::Uniform;
            propsUboDesc.access = gfx::BufferAccess::Dynamic;
            propsUboDesc.cpuVisible = true;

            auto uboCRes = gbgfx->CreateBuffer(propsUboDesc);

            if (!uboCRes.has_value()) {
                auto& err = uboCRes.error();
                errors.push_back(ExError{err.GetCode(), 
                    "MaterialProps uniform buffer creation failure at matIdx=" + std::to_string(matIdx) +
                    " Error=" + std::string(err.GetMessage())
                });
            } else {
                propsUbo = uboCRes.value();
                localHeldBuffers.push_back(propsUbo);

                gfx::ResourceHandle uboResH(propsUbo);
                gfx::Binding uboBind;
                uboBind.bindName = AX_BINDING_KEY_MATERIAL_PROPS;
                uboBind.slot = AX_BINDING_SLOT_MATERIAL_PROPS;
                uboBind.type = gfx::BindingType::UniformBuffer;
                uboBind.resources = {std::vector<gfx::ResourceHandle>{uboResH}};
                uboBind.offset = 0;
                uboBind.range = 0;
                uboBind.stageMask = gfx::BindingStage_Vertex | gfx::BindingStage_Fragment;

                bindings.push_back(uboBind);
            }

            auto uboUErr = gbgfx->UpdateBuffer(propsUbo, 0, MAT_PROPS_SIZE, &mat.props);

            if (uboUErr.IsValid()) {
                errors.push_back(ExError{uboUErr.GetCode(), 
                    "MaterialProps uniform buffer upload failure at matIdx=" + std::to_string(matIdx) +
                    " Error=" + std::string(uboUErr.GetMessage())
                });
            }

            for (uint32_t i{0}; i < mat.texture_indices.size(); i++) {
                auto texType = (MaterialTextureType) i;
                auto& texIndices = mat.texture_indices[i];

                for (auto& texIdx : texIndices) {
                    auto& tex = desc.immutableImport.textures[texIdx];

                    gfx::TextureDesc texDesc;
                    texDesc.type = gfx::TextureType::Texture2D;
                    texDesc.width = tex.image.width;
                    texDesc.height = tex.image.height;
                    texDesc.format = GetTexFormatOfImg(tex.image.format);
                    texDesc.usage = gfx::TextureUsage::Sampled;

                    std::vector<utils::URaw> layers;
                    layers.push_back(utils::URaw(utils::URawView(tex.image.bytes.data(), tex.image.bytes.size())));
                    texDesc.pixelsByLayers = {std::move(layers)};

                    gfx::TextureSubDesc subDesc;
                    if (gbgfx->GetCaps().maxAniso >= 4.0f) {
                        subDesc.aniso = 4.0f;
                    }
                    subDesc.generateMips = true; // TODO: add parameters in desc which affets each texture upload and gives ability to code (or we actually) to modify texture/uniform-props desc per material/material-texture
                    subDesc.mipFilter = gfx::MipmapFilter::Linear;
                    subDesc.minFilter = gfx::TextureFilter::Linear;
                    subDesc.magFilter = gfx::TextureFilter::Linear;
                    texDesc.subDesc = std::move(subDesc);

                    auto texRes = gbgfx->CreateTexture(texDesc);

                    if (!texRes.has_value()) {
                        auto& err = texRes.error();
                        errors.push_back(ExError{err.GetCode(), 
                            "Texture Upload failure at matIdx=" + std::to_string(matIdx) +
                            " and texIdx=" + std::to_string(texIdx) + ", Error=" + std::string(err.GetMessage())
                        });
                    } else {
                        auto val = texRes.value();

                        textureHandles.push_back(val);
                        localHeldTextures.push_back(val);

                        gfx::ResourceHandle texResH(val);
                        gfx::Binding texBind;
                        texBind.bindName = GetAssetBindKey(texType);
                        texBind.slot = GetAssetBindSlot(texType);
                        texBind.type = gfx::BindingType::SampledTexture;
                        texBind.resources = {std::vector<gfx::ResourceHandle>{texResH}};
                        texBind.stageMask = gfx::BindingStage_Vertex | gfx::BindingStage_Fragment;

                        bindings.push_back(texBind);
                    }
                }
            }

            gfx::ResourceSetDesc rDesc;
            rDesc.setIndex = argMat.resourceSetIndex;
            rDesc.bindings = {std::move(bindings)};

            auto resRes = gbgfx->CreateResourceSet(rDesc);
            if (!resRes.has_value()) {
                auto& err = resRes.error();
                errors.push_back(ExError{err.GetCode(), 
                    "ResourceSet creation failure at matIdx=" + std::to_string(matIdx) +
                    ", Error=" + std::string(err.GetMessage())
                });
                
                for (auto& binding : bindings) {
                    for (auto& res : binding.resources) {
                        switch (res.kind) {
                            case gfx::ResourceKind::Buffer:
                                gbgfx->DestroyBuffer(res.AsBuffer());
                            break;
                            case gfx::ResourceKind::Texture:
                                gbgfx->DestroyTexture(res.AsTexture());
                            break;
                        }
                    }
                }
            } else {
                auto resHandle = resRes.value();
                localHeldResources.push_back(resHandle);

                AssetGpuMaterial gpuMat;

                gpuMat.bindings = std::move(bindings);
                gpuMat.resourcesHandle = resHandle;

                materials.push_back(gpuMat);
            }
        }
        
        if (!errors.empty()) {
            for (auto& material : materials) {
                for (auto& binding : material.bindings) {
                    for (auto& resource : binding.resources) {
                        if (binding.type == gfx::BindingType::UniformBuffer || binding.type == gfx::BindingType::StorageBuffer) {
                            gbgfx->DestroyBuffer(resource.AsBuffer());
                        } else { // if (binding.type == Texture)
                            gbgfx->DestroyTexture(resource.AsTexture());
                        }
                    }
                }
                gbgfx->DestroyResourceSet(material.resourcesHandle);
            }
            std::stringstream error_str_stream;
            for (auto& error : errors) {
                error_str_stream << "ErrCode=" << error.GetCode() << ", "
                    << error.GetMessage() << "\n";
            }
            return ExError{error_str_stream.str()};
        }

        m_HeldBuffers.insert(
            m_HeldBuffers.end(),
            std::make_move_iterator(localHeldBuffers.begin()),
            std::make_move_iterator(localHeldBuffers.end())
        );

        m_HeldTextures.insert(
            m_HeldTextures.end(),
            std::make_move_iterator(localHeldTextures.begin()),
            std::make_move_iterator(localHeldTextures.end())
        );

        m_HeldResources.insert(
            m_HeldResources.end(),
            std::make_move_iterator(localHeldResources.begin()),
            std::make_move_iterator(localHeldResources.end())
        );

        return AssetGpuMaterials{{std::move(materials)}};
    });
}

gfx::TextureFormat GetTexFormatOfImg(const gfx::ImageFormat& fmt) {
    switch (fmt) {
        case gfx::ImageFormat::Raw_R8:          return gfx::TextureFormat::R8_UNORM;
        case gfx::ImageFormat::Raw_RG8:         return gfx::TextureFormat::RG8_UNORM;
        case gfx::ImageFormat::Raw_RGB8:        return gfx::TextureFormat::RGB8_UNORM;
        case gfx::ImageFormat::Raw_RGBA8:       return gfx::TextureFormat::RGBA8_UNORM;
        
        case gfx::ImageFormat::Raw_R16:         return gfx::TextureFormat::R16_UNORM;
        case gfx::ImageFormat::Raw_RG16:        return gfx::TextureFormat::RG16_UNORM;
        case gfx::ImageFormat::Raw_RGB16:       return gfx::TextureFormat::RGB16_UNORM;
        case gfx::ImageFormat::Raw_RGBA16:      return gfx::TextureFormat::RGBA16_UNORM;
        
        case gfx::ImageFormat::Raw_R32F:        return gfx::TextureFormat::R32_FLOAT;
        case gfx::ImageFormat::Raw_RG32F:       return gfx::TextureFormat::RG32_FLOAT;
        case gfx::ImageFormat::Raw_RGB32F:      return gfx::TextureFormat::RGB32_FLOAT;
        case gfx::ImageFormat::Raw_RGBA32F:     return gfx::TextureFormat::RGBA32_FLOAT;

        case gfx::ImageFormat::Compressed_RGBA_ASTC_4x4:    return gfx::TextureFormat::ASTC_4x4_UNORM;
        case gfx::ImageFormat::Compressed_RGBA_ASTC_6x6:    return gfx::TextureFormat::ASTC_6x6_UNORM;
        case gfx::ImageFormat::Compressed_RGBA_ASTC_8x8:    return gfx::TextureFormat::ASTC_8x8_UNORM;
        case gfx::ImageFormat::Compressed_RGB_DXT1:         return gfx::TextureFormat::BC1_UNORM;
        case gfx::ImageFormat::Compressed_RGBA_DXT5:        return gfx::TextureFormat::BC3_UNORM;
    }
}

const char* GetAssetBindKey(const MaterialTextureType& type) {
    switch (type) {
        case MaterialTextureType_Albedo:            return AX_BINDING_KEY_TEXTURE_BASE_COLOR;
        case MaterialTextureType_Specular:          return AX_BINDING_KEY_TEXTURE_SPECULAR;
        case MaterialTextureType_NormalMap:         return AX_BINDING_KEY_TEXTURE_NORMAL;
        case MaterialTextureType_HeightMap:         return AX_BINDING_KEY_TEXTURE_HEIGHTMAP;
        case MaterialTextureType_Roughness:         return AX_BINDING_KEY_TEXTURE_ROUGHNESS;
        case MaterialTextureType_Metallic:          return AX_BINDING_KEY_TEXTURE_METALLIC;
        case MaterialTextureType_Emissive:          return AX_BINDING_KEY_TEXTURE_EMISSIVE;
        case MaterialTextureType_AmbientOcclusion:  return AX_BINDING_KEY_TEXTURE_AO;
        case MaterialTextureType_Displacement:      return AX_BINDING_KEY_TEXTURE_DISPLACEMENT;
    }
    return "";
}

uint32_t GetAssetBindSlot(const MaterialTextureType& type) {
    switch (type) {
        case MaterialTextureType_Albedo:            return AX_BINDING_SLOT_TEXTURE_BASE_COLOR;
        case MaterialTextureType_Specular:          return AX_BINDING_SLOT_TEXTURE_SPECULAR;
        case MaterialTextureType_NormalMap:         return AX_BINDING_SLOT_TEXTURE_NORMAL;
        case MaterialTextureType_HeightMap:         return AX_BINDING_SLOT_TEXTURE_HEIGHTMAP;
        case MaterialTextureType_Roughness:         return AX_BINDING_SLOT_TEXTURE_ROUGHNESS;
        case MaterialTextureType_Metallic:          return AX_BINDING_SLOT_TEXTURE_METALLIC;
        case MaterialTextureType_Emissive:          return AX_BINDING_SLOT_TEXTURE_EMISSIVE;
        case MaterialTextureType_AmbientOcclusion:  return AX_BINDING_SLOT_TEXTURE_AO;
        case MaterialTextureType_Displacement:      return AX_BINDING_SLOT_TEXTURE_DISPLACEMENT;
    }
    return UINT32_MAX;
}

}