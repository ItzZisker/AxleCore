#ifdef __AX_ASSETS_ASSIMP__
#pragma once

#include "axle/assets/AX_AssetImporter.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <filesystem>

namespace axle::assets
{

class AssetSTLAssimpFileImporter : public IAssetImporter {
public:
    explicit AssetSTLAssimpFileImporter(const std::filesystem::path& path);

    utils::ExResult<AssetImportResult> Import() override;

    std::string GetImporterName() const override {
        return "AssimpImporter";
    }

private:
    std::filesystem::path m_Path;

private:
    void ProcessNode(
        const aiNode* node,
        const aiScene* scene,
        Node& outNode,
        AssetImportResult& result
    );

    uint32_t ProcessMesh(
        const aiMesh* mesh,
        const aiScene* scene,
        AssetImportResult& result
    );

    uint32_t ProcessMaterial(
        const aiMaterial* material,
        const aiScene* scene,
        AssetImportResult& result
    );
};

}
#endif