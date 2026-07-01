#ifdef __AX_ASSETS_ASSIMP__
#pragma once

#include "axle/assets/AX_AssetImporter.hpp"

#include "axle/utils/AX_Expected.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <filesystem>
#include <vector>

namespace axle::assets
{

class AssetSTLAssimpFileImporter : public IAssetImporter {
public:
    explicit AssetSTLAssimpFileImporter(const AssetImportDesc& desc, const std::filesystem::path& path);

    utils::ExResult<AssetImportResult> Import() override;

    std::string GetImporterName() const override {
        return "AssimpImporter";
    }

    utils::Span<utils::ExError> GetErrors() {
        return {m_Errors.data(), m_Errors.size()};
    }
private:
    std::filesystem::path m_Path;
    std::vector<utils::ExError> m_Errors;

    struct AssimpNodeProcessParams {
        const aiNode* node;
        const aiScene* scene;
        Node& outNode;
        AssetImportResult& result;
        uint32_t& meshIdx;
        uint32_t& buffIdx;
        uint32_t& nodeIdx;
    };

    struct AssimpLightProcessParams {
        const aiScene* scene;
        AssetImportResult& result;
        uint32_t& lightIdx;
    };

    struct AssimpCameraProcessParams {
        const aiScene* scene;
        AssetImportResult& result;
        uint32_t& cameraIdx;
    };

    struct AssimpMeshProcessParams {
        const aiMesh* mesh;
        AssetImportResult& result;
        uint32_t& meshIdx;
        uint32_t& buffIdx;
    };

    struct AssimpMaterialProcessParams {
        const aiScene* scene;
        AssetImportResult& result;
        const uint32_t& matIdx;
        std::vector<AssetTexture>& asset_texs;
    };

    void ProcessNode(const AssimpNodeProcessParams& params);
    void ProcessLights(const AssimpLightProcessParams& params);
    void ProcessCameras(const AssimpCameraProcessParams& params);
    void ProcessMesh(const AssimpMeshProcessParams& params);

    utils::ExError ProcessMaterial(const AssimpMaterialProcessParams& params);
};

}
#endif