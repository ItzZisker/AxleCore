#pragma once

#include "axle/assets/AX_AssetImporter.hpp"

#include "axle/core/concurrency/AX_ThreadCycler.hpp"

#include "axle/graphics/AX_GraphicsParams.hpp"

#include "axle/utils/AX_Universal.hpp"
#include "axle/utils/AX_Expected.hpp"
#include "axle/utils/AX_Types.hpp"

#define AX_BINDING_KEY_MATERIAL_PROPS "_res_materialProps"
#define AX_BINDING_KEY_TEXTURE_BASE_COLOR "_res_textureBaseColor"
#define AX_BINDING_KEY_TEXTURE_SPECULAR "_res_textureSpecular"
#define AX_BINDING_KEY_TEXTURE_NORMAL "_res_textureNormal"
#define AX_BINDING_KEY_TEXTURE_HEIGHTMAP "_res_textureHeightMap"
#define AX_BINDING_KEY_TEXTURE_ROUGHNESS "_res_textureRoughness"
#define AX_BINDING_KEY_TEXTURE_METALLIC "_res_textureMetallic"
#define AX_BINDING_KEY_TEXTURE_AO "_res_textureAO"
#define AX_BINDING_KEY_TEXTURE_DISPLACEMENT "_res_textureDisplacement"
#define AX_BINDING_KEY_TEXTURE_EMISSIVE "_res_textureEmissive"

#define AX_BINDING_SLOT_MATERIAL_PROPS 0
#define AX_BINDING_SLOT_TEXTURE_BASE_COLOR 1
#define AX_BINDING_SLOT_TEXTURE_SPECULAR 2
#define AX_BINDING_SLOT_TEXTURE_NORMAL 3
#define AX_BINDING_SLOT_TEXTURE_HEIGHTMAP 4
#define AX_BINDING_SLOT_TEXTURE_ROUGHNESS 5
#define AX_BINDING_SLOT_TEXTURE_METALLIC 6
#define AX_BINDING_SLOT_TEXTURE_AO 7
#define AX_BINDING_SLOT_TEXTURE_DISPLACEMENT 8
#define AX_BINDING_SLOT_TEXTURE_EMISSIVE 9

namespace axle::assets
{

gfx::TextureFormat GetTexFormatOfImg(const gfx::ImageFormat& fmt);

const char* GetAssetBindKey(const MaterialTextureType& type);
uint32_t GetAssetBindSlot(const MaterialTextureType& type);

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