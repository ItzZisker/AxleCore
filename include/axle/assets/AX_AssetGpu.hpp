#pragma once

#include "axle/assets/AX_AssetImporter.hpp"

#include "axle/core/concurrency/AX_ThreadCycler.hpp"

#include "axle/graphics/AX_GraphicsParams.hpp"

#include "axle/utils/AX_Universal.hpp"
#include "axle/utils/AX_Expected.hpp"
#include "axle/utils/AX_Types.hpp"

namespace axle::assets
{

struct AssetGpuMesh {
    gfx::MeshVertexLayout layout;

    gfx::BufferHandle vertices;
    gfx::BufferHandle indices;

    bool indexed{false};
};

struct AssetGpuMaterial {
    utils::CowSpan<gfx::Binding> bindings;
    utils::Range<uint32_t> bindingsRange;

    gfx::ResourceSetHandle resourcesHandle;
};

class AssetGpu {
private:
    SharedPtr<core::ThreadContextGfx> m_GfxThread;

    std::unordered_map<gfx::ResourceSetHandle, gfx::ResourceSetDesc> m_ResourcesDescs;
    std::unordered_map<gfx::TextureHandle, gfx::TextureDesc> m_TextureDescs;
    std::unordered_map<gfx::BufferHandle, gfx::BufferDesc> m_BufferDescs;
public:
    explicit AssetGpu(SharedPtr<core::ThreadContextGfx> gfxThread);
    ~AssetGpu();

    utils::ExResult<AssetGpuMesh> UploadMesh(const AssetMesh& mesh);
    utils::ExResult<AssetGpuMaterial> UploadMaterial(uint32_t bindRangeOffset, const AssetMaterial& mat);
    
    utils::ExResult<gfx::ResourceSetDesc> DescribeBindings(const gfx::ResourceSetHandle handle);
    utils::ExResult<gfx::TextureDesc> DescribeTexture(const gfx::TextureHandle& handle);
    utils::ExResult<gfx::BufferDesc> DescribeBuffer(const gfx::BufferHandle& handle);
};

}