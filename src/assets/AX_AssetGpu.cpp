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
    if (m_GfxThread->ValidateThread()) {
        lambdaFinalizer();
    } else {
        m_GfxThread->EnqueueTask(lambdaFinalizer);
    }
}

utils::ExResult<AssetGpuMesh> AssetGpu::UploadMesh(const AssetMesh& mesh) {

}

utils::ExResult<AssetGpuMaterial> AssetGpu::UploadMaterial(uint32_t bindOffset, const AssetMaterial& mat) {

}

utils::ExResult<gfx::BufferDesc> AssetGpu::DescribeBuffer(const gfx::BufferHandle&) {

}

}