#pragma once

#include "axle/assets/AX_AssetImporter.hpp"

#include "axle/core/concurrency/AX_ThreadCycler.hpp"

#include "axle/graphics/AX_GraphicsParams.hpp"

#include "axle/utils/AX_Universal.hpp"
#include "axle/utils/AX_Expected.hpp"
#include "axle/utils/AX_Types.hpp"

namespace axle::assets
{

gfx::TextureFormat GetTexFormatOfImg(const gfx::ImageFormat& fmt);

struct AssetGpuMesh {
    gfx::MeshVertexLayout layout;

    gfx::BufferHandle vertices;
    gfx::BufferHandle indices;

    bool indexed{false};
};

struct AssetGpuMeshes {
    utils::CowSpan<AssetGpuMesh> meshes;
};

struct AssetGpuMaterial {
    utils::CowSpan<gfx::Binding> bindings;
    utils::Range<uint32_t> bindingsRange;

    gfx::ResourceSetHandle resourcesHandle;
};

struct AssetGpuMaterials {
    utils::CowSpan<AssetGpuMaterial> materials;
};

struct AssetMeshesUploadDesc {
    const AssetImportResult& immutableImport;
    const utils::CowSpan<AssetMesh>& immutableMeshes;
};

struct AssetMaterialDesc {
    const AssetMaterial& immutableMaterial;
    uint32_t resourceSetIndex{0};
};

struct AssetMaterialsUploadDesc {
    const AssetImportResult& immutableImport;
    const utils::CowSpan<AssetMaterialDesc>& immutableMaterials;
};

class AssetGpu {
private:
    SharedPtr<core::ThreadContextGfx> m_GfxThread;

    std::unordered_map<gfx::ResourceSetHandle, gfx::ResourceSetDesc> m_ResourcesDescs;
    std::unordered_map<gfx::TextureHandle, gfx::TextureDesc> m_TextureDescs;
    std::unordered_map<gfx::BufferHandle, gfx::BufferDesc> m_BufferDescs;

    std::mutex m_Mutex{};
public:
    explicit AssetGpu(SharedPtr<core::ThreadContextGfx> gfxThread);
    ~AssetGpu();

    Future<utils::ExResult<AssetGpuMeshes>> UploadMeshes(AssetMeshesUploadDesc& desc);
    Future<utils::ExResult<AssetGpuMaterials>> UploadMaterials(AssetMaterialsUploadDesc& desc);
    
    utils::ExResult<gfx::ResourceSetDesc> DescribeBindings(const gfx::ResourceSetHandle& handle);
    utils::ExResult<gfx::TextureDesc> DescribeTexture(const gfx::TextureHandle& handle);
    utils::ExResult<gfx::BufferDesc> DescribeBuffer(const gfx::BufferHandle& handle);
};

}