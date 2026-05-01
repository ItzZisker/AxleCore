#include "axle/assets/AX_AssetSTLAssimpFileImporter.hpp"
#include "axle/assets/AX_AssetAssimpDefs.hpp"

#include "axle/utils/AX_Universal.hpp"

namespace axle::assets
{

AssetSTLAssimpFileImporter::AssetSTLAssimpFileImporter(uint32_t assetImportFlags, const std::filesystem::path& path)
    : IAssetImporter(assetImportFlags), m_Path(path) {}

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

    Node root;
    root.name = "ROOT";

    ProcessNode(scene->mRootNode, scene, root, result);
    result.nodes = { std::vector<Node>{root} };

    return result;
}

void AssetSTLAssimpFileImporter::ProcessNode(
    const aiNode* node,
    const aiScene* scene,
    Node& outNode,
    AssetImportResult& result
) {
    outNode.name = node->mName.C_Str();
    outNode.transform = utils::Coordination{utils::Assimp_ToGLM(node->mTransformation)};

    result.buffers = utils::CowSpan(std::vector<AssetBuffer>(2 * scene->mNumMeshes));
    result.materials = utils::CowSpan(std::vector<AssetMaterial>(scene->mNumMaterials));

    uint32_t meshIdx{0}, buffIdx{0};
    for (uint32_t i = 0; i < node->mNumMeshes; ++i) {
        ProcessMesh(scene->mMeshes[node->mMeshes[i]], scene, result, meshIdx, buffIdx);
        outNode.meshId = meshIdx;
    }
    for (uint32_t i = 0; i < node->mNumChildren; ++i) {
        Node child;
        ProcessNode(node->mChildren[i], scene, child, result);
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
    const aiScene* scene,
    AssetImportResult& result,
    uint32_t& meshIdx,
    uint32_t& buffIdx
) {
    std::vector<uint8_t> vertexBuffer;
    auto fmt = GetVertexFormat(GetMeshUvCount(mesh), HasFlag(AssetImportFlag::CalcTangents));
    auto fmtDesc = GetVertexFormatDesc(fmt);
    vertexBuffer.reserve(sizeof(float) * ((fmtDesc.hasTangents ? 6 : 0) + fmtDesc.uvCount * 2 + 6) * mesh->mNumVertices);

    std::vector<uint8_t> indexBuffer;
    indexBuffer.reserve(sizeof(uint32_t) * mesh->mNumFaces);

    auto vertexStream = data::BufferDataStream(utils::URawView(vertexBuffer.data(), vertexBuffer.size()));
    auto indexStream = data::BufferDataStream(utils::URawView(indexBuffer.data(), indexBuffer.size()));

    for (uint32_t i{0}; i < mesh->mNumVertices; ++i) {
        vertexStream.Write(&mesh->mVertices[i], sizeof(float) * 3);
        vertexStream.Write(&mesh->mNormals[i], sizeof(float) * 3);

        for (uint32_t j{0}; j < fmtDesc.uvCount; j++) {
            if (mesh->mTextureCoords[j]) {
                vertexStream.Write(&mesh->mTextureCoords[j][i], sizeof(float) * 2);
            } else {
                static const float zeros[2]{0.0f, 0.0f};
                vertexStream.Write(&zeros, sizeof(float) * 2);
            }            
        }
    }

    for (uint32_t i = 0; i < mesh->mNumFaces; ++i) {
        const aiFace& face = mesh->mFaces[i];

        for (uint32_t j = 0; j < face.mNumIndices; ++j)
            indexStream.Write(&face.mIndices[j], sizeof(uint32_t) * 2);
    }

    AssetBuffer vertexBuf;
    vertexBuf.type = AssetBufferType::Vertex;
    vertexBuf.stride = fmtDesc.stride;
    vertexBuf.count = mesh->mNumVertices;
    vertexBuf.raw = {std::move(vertexBuffer)};

    AssetBuffer indexBuf;
    indexBuf.type = AssetBufferType::Index;
    indexBuf.stride = sizeof(uint32_t);
    indexBuf.count = indexBuffer.size();
    indexBuf.raw = {std::move(indexBuffer)};

    result.buffers[buffIdx++] = std::move(vertexBuf);
    result.buffers[buffIdx++] = std::move(indexBuf);

    AssetMesh assetMesh;
    assetMesh.vertices = vertexBuf.External();
    assetMesh.indices = indexBuf.External();

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

void AssetSTLAssimpFileImporter::ProcessMaterial(
    const aiMaterial* material,
    const aiScene* scene,
    AssetImportResult& result,
    uint32_t& matIdx
) {
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

    auto LoadTexture = [&](aiTextureType type) -> int {
        if (material->GetTextureCount(type) == 0)
            return -1;

        aiString path;
        if (material->GetTexture(type, 0, &path) != AI_SUCCESS)
            return -1;

        const aiTexture* tex = scene->GetEmbeddedTexture(path.C_Str());
        if (tex) {
            auto& t = result.textures.emplace_back();
            t.name = path.C_Str();
            t.width = tex->mWidth;
            t.height = tex->mHeight;
            t.data.assign(tex->pcData, tex->pcData + tex->mWidth * tex->mHeight);
            return (int)(result.textures.size() - 1);
        }

        auto& t = result.textures.emplace_back();
        t.filePath = m_Path.parent_path() / std::string(path.C_Str());
        return (int)(result.textures.size() - 1);
    };

    mat.diffuseTexture             = LoadTexture(aiTextureType_DIFFUSE);
    mat.normalTexture              = LoadTexture(aiTextureType_NORMALS);
    mat.metallicRoughnessTexture   = LoadTexture(aiTextureType_METALNESS);

    mat.imported = true;
    result.materials[matIdx] = mat;
    return (uint32_t)result.materials.size() - 1;
}


}