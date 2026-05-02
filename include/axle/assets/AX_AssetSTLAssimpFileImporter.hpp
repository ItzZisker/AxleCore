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

private:
    void ProcessNode(
        const aiNode* node,
        const aiScene* scene,
        Node& outNode,
        AssetImportResult& result
    );

    void ProcessMesh(
        const aiMesh* mesh,
        const aiScene* scene,
        AssetImportResult& result,
        uint32_t& meshIdx,
        uint32_t& buffIdx
    );

    void ProcessMaterial(
        const aiMaterial* material,
        const aiScene* scene,
        AssetImportResult& result,
        const uint32_t& matIdx,
        std::vector<AssetTexture>& asset_texs
    );

    void ProcessLights(
        const aiScene* scene,
        AssetImportResult& result,
        uint32_t& lightIdx
    );

    void ProcessCameras(
        const aiScene* scene,
        AssetImportResult& result,
        uint32_t& cameraIdx
    );
};

}
#endif