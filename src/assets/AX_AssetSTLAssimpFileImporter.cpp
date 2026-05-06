#include "axle/assets/AX_AssetSTLAssimpFileImporter.hpp"
#include "axle/assets/AX_AssetAssimpDefs.hpp"

#include "axle/utils/AX_Universal.hpp"

#include <iostream>

namespace axle::assets
{

AssetSTLAssimpFileImporter::AssetSTLAssimpFileImporter(const AssetImportDesc& desc, const std::filesystem::path& path)
    : IAssetImporter(desc), m_Path(path) {}

utils::ExResult<AssetImportResult> AssetSTLAssimpFileImporter::Import() {
    Assimp::Importer importer;

    auto flags = aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ImproveCacheLocality;
    
    if (HasFlag(AssetImportFlag::CalcTangents)) {
        flags |= aiProcess_CalcTangentSpace;
    }

    const aiScene* scene = importer.ReadFile(m_Path.string(), flags);

    if (!scene || !scene->mRootNode)
        return utils::ExError("Assimp failed to load scene");

    AssetImportResult result;

    std::vector<AssetTexture> asset_texs;
    result.materials = {std::vector<AssetMaterial>(scene->mNumMaterials)};
    for (uint32_t matIdx{0}; matIdx < scene->mNumMaterials; matIdx++) {
        auto err = ProcessMaterial(scene, result, matIdx, asset_texs);
        if (err.IsValid()) return err;
    }
    result.textures = {std::move(asset_texs)};

    Node root;
    root.name = "ROOT";

    uint32_t meshIdx{0}, buffIdx{0};
    result.meshes = {std::vector<AssetMesh>(scene->mNumMeshes)};
    result.buffers = {std::vector<AssetBuffer>(2 * scene->mNumMeshes)};
    ProcessNode(scene->mRootNode, scene, root, result, meshIdx, buffIdx);
    result.nodes = {std::vector<Node>{root}};

    return result;
}

void AssetSTLAssimpFileImporter::ProcessNode(
    const aiNode* node,
    const aiScene* scene,
    Node& outNode,
    AssetImportResult& result,
    uint32_t& meshIdx,
    uint32_t& buffIdx
) {
    outNode.name = node->mName.C_Str();
    outNode.transform = utils::Coordination{utils::Assimp_ToGLM(node->mTransformation)};

    for (uint32_t i = 0; i < node->mNumMeshes; ++i) {
        outNode.meshId = meshIdx;
        ProcessMesh(scene->mMeshes[node->mMeshes[i]], result, meshIdx, buffIdx);
    }
    for (uint32_t i = 0; i < node->mNumChildren; ++i) {
        Node child;
        ProcessNode(node->mChildren[i], scene, child, result, meshIdx, buffIdx);
        outNode.children.push_back(std::move(child));
    }
}

uint32_t GetMeshUvCount(const aiMesh* mesh) {
    uint32_t count = 0;

    for (uint32_t i = 0; i < 8; i++) {
        if (mesh->mTextureCoords[i] != nullptr)
            count++;
    }
    return count;
}

void AssetSTLAssimpFileImporter::ProcessMesh(
    const aiMesh* mesh,
    AssetImportResult& result,
    uint32_t& meshIdx,
    uint32_t& buffIdx
) {
    std::vector<uint8_t> vertexBuffer;
    auto fmt = GetVertexFormat(GetMeshUvCount(mesh), HasFlag(AssetImportFlag::CalcTangents));
    auto fmtDesc = GetVertexFormatDesc(fmt);
    auto vtSize = sizeof(float) * ((fmtDesc.hasTangents ? 6 : 0) + fmtDesc.uvCount * 2 + 6) * mesh->mNumVertices;
    vertexBuffer.reserve(vtSize);
    vertexBuffer.resize(vtSize);

    std::vector<uint8_t> indexBuffer;
    auto idxCount{0u};
    for (uint32_t i{0}; i < mesh->mNumFaces; i++) {
        idxCount += mesh->mFaces[i].mNumIndices;
    }
    auto idxSize = sizeof(uint32_t) * idxCount;
    indexBuffer.reserve(idxSize);
    indexBuffer.resize(idxSize);

    auto vertexStream = data::BufferDataStream(utils::URawView(vertexBuffer.data(), vtSize));
    auto indexStream = data::BufferDataStream(utils::URawView(indexBuffer.data(), idxSize));

    vertexStream.Open();
    indexStream.Open();

    for (uint32_t i{0}; i < mesh->mNumVertices; ++i) {
        vertexStream.Write(&mesh->mVertices[i], sizeof(float) * 3);
        vertexStream.Write(&mesh->mNormals[i], sizeof(float) * 3);

        for (uint32_t j{0}; j < fmtDesc.uvCount; j++) {
            if (mesh->mTextureCoords[j]) {
                vertexStream.Write(&mesh->mTextureCoords[j][i], sizeof(float) * 2);
            } else {
                static const float zeros2[2]{0.0f, 0.0f};
                vertexStream.Write(zeros2, sizeof(float) * 2);
            }
        }

        if (fmtDesc.hasTangents) {
            if (mesh->mTangents) {
                vertexStream.Write(&mesh->mTangents[i], sizeof(float) * 3);
            } else {
                static const float zeros3[3]{0.0f, 0.0f, 0.0f};
                vertexStream.Write(zeros3, sizeof(float) * 3);
            }

            if (mesh->mBitangents) {
                vertexStream.Write(&mesh->mBitangents[i], sizeof(float) * 3);
            } else {
                static const float zeros3[3]{0.0f, 0.0f, 0.0f};
                vertexStream.Write(zeros3, sizeof(float) * 3);
            }
        }
    }

    for (uint32_t i = 0; i < mesh->mNumFaces; ++i) {
        const aiFace& face = mesh->mFaces[i];

        for (uint32_t j = 0; j < face.mNumIndices; ++j)
            indexStream.Write(&face.mIndices[j], sizeof(uint32_t));
    }

    AssetBuffer vertexBuf;
    vertexBuf.type = AssetBufferType::Vertex;
    vertexBuf.stride = fmtDesc.stride;
    vertexBuf.count = mesh->mNumVertices;
    vertexBuf.raw = {std::move(vertexBuffer)};

    AssetBuffer indexBuf;
    indexBuf.type = AssetBufferType::Index;
    indexBuf.stride = sizeof(uint32_t);
    indexBuf.count = idxCount;
    indexBuf.raw = {std::move(indexBuffer)};

    uint32_t vertBuffIdx{buffIdx++}, idxBuffIdx{buffIdx++};

    result.buffers[vertBuffIdx] = {std::move(vertexBuf)};
    result.buffers[idxBuffIdx] = {std::move(indexBuf)};

    AssetMesh assetMesh;
    assetMesh.vertexBufferIdx = vertBuffIdx;
    assetMesh.indexBufferIdx = idxBuffIdx;
    assetMesh.materialIdx = mesh->mMaterialIndex;
    // TODO: Research: about how to get mesh parts

    result.meshes[meshIdx++] = std::move(assetMesh);
}

glm::vec4 GetMemberColor(
    const aiMaterial* pMaterial, const char* pAiMatKey,
    int AiMatType, int AiMatIdx
) {
    glm::vec4 color;
    aiColor4D aiColor(0.0f, 0.0f, 0.0f, 0.0f);

    if (pMaterial->Get(pAiMatKey, AiMatType, AiMatIdx, aiColor) == AI_SUCCESS) {
        color.x = aiColor.r;
        color.y = aiColor.g;
        color.z = aiColor.b;
        color.w = std::min(aiColor.a, 1.0f);
    } else {
        color = glm::vec4(1.0f);
    }
    return color;
}

void SwapAssimp_BGRA8888_To_RGBA8888(const aiTexture* assimp_tex) {
    auto width = assimp_tex->mWidth;
    auto height = assimp_tex->mHeight;

    for (uint32_t i{0}; i < width * height; i++) {
        aiTexel& texel = assimp_tex->pcData[i];
        aiTexel temp = texel;

        texel.b = temp.r;
        texel.r = temp.b;
    }
}

utils::ExError AssetSTLAssimpFileImporter::ProcessMaterial(
    const aiScene* scene,
    AssetImportResult& result,
    const uint32_t& matIdx,
    std::vector<AssetTexture>& asset_texs
) {
    auto material = scene->mMaterials[matIdx];
    AssetMaterial mat{};
    mat.name = material->GetName().C_Str();

    mat.props.baseColor = GetMemberColor(material, AI_MATKEY_BASE_COLOR);
    mat.props.diffuseColor = GetMemberColor(material, AI_MATKEY_COLOR_DIFFUSE);
    mat.props.specularColor = GetMemberColor(material, AI_MATKEY_COLOR_SPECULAR);
    
    float ior = 1.0f;
    float opacity = 1.0f;
    
    if (material->Get(AI_MATKEY_REFRACTI, ior) == AI_SUCCESS) {
        mat.props.ior = glm::vec3(ior);
    }
    if (material->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS) {
        mat.props.opacity = opacity;
        mat.props.maxOpacity = opacity;
        mat.props.isTransparent = (opacity < 1.0f);

        if (HasFlag(AssetImportFlag::IncludePBR)) {
            mat.props.pbr.transparencyFactor = std::clamp(1.0f - opacity, 0.0f, 1.0f);
            if (mat.props.pbr.transparencyFactor >= 1.0f - m_Desc.opaquenessThreshold) {
                mat.props.pbr.transparencyFactor = 0.0f;
            }
        }
    }

    float alphaCutoff{0.0f};
    if (HasFlag(AssetImportFlag::IncludePBR)) {
        mat.props.pbr.emissiveColor = GetMemberColor(material, AI_MATKEY_COLOR_EMISSIVE);
        if (material->Get(AI_MATKEY_GLTF_ALPHACUTOFF, alphaCutoff) == AI_SUCCESS) {
            mat.props.pbr.emissiveColor.w = alphaCutoff;
        } else {
            mat.props.pbr.emissiveColor.w = 0.5f;                
        }
    }

    if (HasFlag(AssetImportFlag::IncludePBR)) {
        float MetallicFactor;
        if (material->Get(AI_MATKEY_METALLIC_FACTOR, MetallicFactor) == AI_SUCCESS) {
            mat.props.pbr.metallicFactor = MetallicFactor;
        }
        float RoughnessFactor;
        if (material->Get(AI_MATKEY_ROUGHNESS_FACTOR, RoughnessFactor) == AI_SUCCESS) {
            mat.props.pbr.roughnessFactor = RoughnessFactor;
        }
        float NormalScale;
        if (material->Get(AI_MATKEY_GLTF_TEXTURE_SCALE(aiTextureType_NORMALS, 0), NormalScale) == AI_SUCCESS) {
            mat.props.pbr.normalScale = NormalScale;
        }
        float OcclusionStrength;
        if (material->Get(AI_MATKEY_GLTF_TEXTURE_SCALE(aiTextureType_LIGHTMAP, 0), OcclusionStrength) == AI_SUCCESS) {
            mat.props.pbr.occlusionStrength = OcclusionStrength;
        }
    }
    material->Get(AI_MATKEY_SHININESS, mat.props.shininess);

    auto MapTexture = [&](MaterialTextureType axType, aiTextureType type) -> utils::ExError {
        uint32_t count;
        if ((count = material->GetTextureCount(type)) == 0) {
            return utils::ExError::NoError();
        }

        aiString path;
        std::vector<uint8_t> tex_indices;
        for (uint32_t i{0}; i < count; i++) {
            if (material->GetTexture(type, i, &path) != AI_SUCCESS) {
                tex_indices.push_back(0);
                continue;
            }
            const aiTexture* assimp_tex{nullptr};

            if (path.length > 1 && path.data[0] == '*') { // Referencing an embedded texture via scene->mTextures
                assimp_tex = scene->mTextures[std::atoi(&path.data[1])];
            } else { // Referencing an embedded texture via path
                assimp_tex = scene->GetEmbeddedTexture(path.C_Str());
            }
            
            auto& asset_tex = asset_texs.emplace_back();
            uint32_t asset_tex_id = asset_texs.size() - 1;

            asset_tex.id = asset_tex_id;
            asset_tex.path = std::string(path.C_Str());

            mat.textures_count[(uint32_t)axType]++;
            mat.textures_idx[(uint32_t)axType].push_back(asset_tex_id);

            if (assimp_tex) { // Texture is embedded
                auto width = assimp_tex->mWidth;
                auto height = assimp_tex->mHeight;

                if (height == 0) { // Texture is compressed (PNG, JPG, etc.)
                    AX_DECL_OR_PROPAGATE(img, gfx::Img_Auto_LoadFileBytes(utils::URawView((uint8_t*)assimp_tex->pcData, width)));
                    asset_tex.image = std::move(img);
                } else {
                    // Texture is raw BGRA888 data (bgra, bgra, bgra...)
                    auto& image = asset_tex.image;
                    image.width = width;
                    image.height = height;
                    image.format = gfx::ImageFormat::Raw_RGBA8;
                    image.bytes = {std::vector<uint8_t>(4 * width * height)};

                    // Swap channels R <=> B
                    SwapAssimp_BGRA8888_To_RGBA8888(assimp_tex);
                    std::memcpy(image.bytes.data(), assimp_tex->pcData, image.bytes.size());
                }
            } else { // Texture is NOT embedded, we should look into filesystem
                AX_DECL_OR_PROPAGATE(img, gfx::Img_Auto_LoadFile({m_Path.parent_path() / path.C_Str()}));
                asset_tex.image = std::move(img);
            }
        }
        return utils::ExError::NoError();
    };

    AX_PROPAGATE_ERROR(MapTexture(MaterialTextureType::Albedo, aiTextureType_DIFFUSE));
    AX_PROPAGATE_ERROR(MapTexture(MaterialTextureType::Specular, aiTextureType_SPECULAR));
    AX_PROPAGATE_ERROR(MapTexture(MaterialTextureType::NormalMap, aiTextureType_NORMALS));
    AX_PROPAGATE_ERROR(MapTexture(MaterialTextureType::HeightMap, aiTextureType_HEIGHT));
    AX_PROPAGATE_ERROR(MapTexture(MaterialTextureType::Roughness, aiTextureType_DIFFUSE_ROUGHNESS));
    AX_PROPAGATE_ERROR(MapTexture(MaterialTextureType::Metallic, aiTextureType_METALNESS));
    AX_PROPAGATE_ERROR(MapTexture(MaterialTextureType::Emissive, aiTextureType_EMISSIVE));
    AX_PROPAGATE_ERROR(MapTexture(MaterialTextureType::AmbientOcclusion, aiTextureType_AMBIENT_OCCLUSION));
    AX_PROPAGATE_ERROR(MapTexture(MaterialTextureType::Displacement, aiTextureType_DISPLACEMENT));

    mat.imported = true;
    result.materials[matIdx] = mat;
    
    return utils::ExError::NoError();
}


}