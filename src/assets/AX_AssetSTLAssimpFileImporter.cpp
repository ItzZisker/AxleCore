#include "axle/assets/AX_AssetSTLAssimpFileImporter.hpp"

#include "axle/utils/AX_Universal.hpp"

namespace axle::assets {

AssetSTLAssimpFileImporter::AssetSTLAssimpFileImporter(const std::filesystem::path& path) : m_Path(path) {}

utils::ExResult<AssetImportResult> AssetSTLAssimpFileImporter::Import() {
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(
        m_Path.string(),
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ImproveCacheLocality
    );

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

    for (uint32_t i = 0; i < node->mNumMeshes; ++i) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        uint32_t meshId = ProcessMesh(mesh, scene, result);

        outNode.meshId = meshId;
    }

    for (uint32_t i = 0; i < node->mNumChildren; ++i) {
        Node child;
        ProcessNode(node->mChildren[i], scene, child, result);
        outNode.children.push_back(std::move(child));
    }
}

uint32_t AssetSTLAssimpFileImporter::ProcessMesh(
    const aiMesh* mesh,
    const aiScene* scene,
    AssetImportResult& result
) {
    std::vector<float> vertexBuffer;
    std::vector<uint32_t> indexBuffer;

    for (uint32_t i = 0; i < mesh->mNumVertices; ++i) {
        const aiVector3D& p = mesh->mVertices[i];
        const aiVector3D& n = mesh->mNormals[i];

        vertexBuffer.push_back(p.x);
        vertexBuffer.push_back(p.y);
        vertexBuffer.push_back(p.z);

        vertexBuffer.push_back(n.x);
        vertexBuffer.push_back(n.y);
        vertexBuffer.push_back(n.z);

        if (mesh->mTextureCoords[0]) {
            vertexBuffer.push_back(mesh->mTextureCoords[0][i].x);
            vertexBuffer.push_back(mesh->mTextureCoords[0][i].y);
        } else {
            vertexBuffer.push_back(0);
            vertexBuffer.push_back(0);
        }
    }

    for (uint32_t i = 0; i < mesh->mNumFaces; ++i) {
        const aiFace& face = mesh->mFaces[i];

        for (uint32_t j = 0; j < face.mNumIndices; ++j)
            indexBuffer.push_back(face.mIndices[j]);
    }

    AssetBuffer vertexBuf;
    vertexBuf.type = AssetBufferType::Vertex;
    vertexBuf.stride = sizeof(float) * 8;
    vertexBuf.count = mesh->mNumVertices;
    vertexBuf.raw = utils::URaw(
        reinterpret_cast<uint8_t*>(vertexBuffer.data()),
        vertexBuffer.size() * sizeof(float)
    );

    AssetBuffer indexBuf;
    indexBuf.type = AssetBufferType::Index;
    indexBuf.stride = sizeof(uint32_t);
    indexBuf.count = indexBuffer.size();

    indexBuf.raw = utils::URaw(
        reinterpret_cast<uint8_t*>(indexBuffer.data()),
        indexBuffer.size() * sizeof(uint32_t)
    );

    result.buffers.push_back(vertexBuf);
    result.buffers.push_back(indexBuf);

    AssetMesh assetMesh;
    assetMesh.vertices = vertexBuf.GetHandle();
    assetMesh.indices = indexBuf.GetHandle();

    result.meshes.push_back(assetMesh);

    return result.meshes.size() - 1;
}

}