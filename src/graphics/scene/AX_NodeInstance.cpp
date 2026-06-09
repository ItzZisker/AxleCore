#include "axle/graphics/scene/AX_NodeInstance.hpp"

namespace axle::scene
{

// struct NodeInstanceParams {
//     const assets::AssetImportResult& immutableImport;
//     uint32_t assetNodeIdx;
// };

// TODO: NodeInstance : public SWFrustumDiscardable
// class NodeInstance {
// private:
//     NodeInstanceParams m_Params;

//     utils::Coordination m_Coords;

//     std::vector<NodeInstance> m_Children{};
//     std::vector<uint32_t> m_MeshesIdx{};

//     bool m_Dirty{false};

//     void Handle(assets::Node& node, glm::vec3& min, glm::vec3& max, bool& unset);
// public:
//     NodeInstance(const NodeInstanceParams& params);
//     ~NodeInstance();

//     assets::Node& GetRoot();

//     bool IsDirty();
//     void MarkDirty(bool flag);

//     inline void ApplyCoords(const std::function<void(utils::Coordination&)>& consumer) {
//         consumer(m_Coords);
//         MarkDirty(true);
//     }

//     const std::vector<NodeInstance>& GetChildren();
//     const std::vector<uint32_t>& GetMeshesIdx();

//     std::string_view GetName();
//     bool HasMeshes();
// };

utils::ExError NodeInstance::GetMinMax(const NodeHandleParams& p) {
    for (const auto& meshId : p.node.meshIds) {
        const auto& mesh = m_Params.immutableImport.meshes[meshId];
        const auto& vertices = m_Params.immutableImport.buffers[mesh.vertexBufferIdx];

        auto stream = data::BufferDataStream(vertices.raw.Get());
        glm::vec3 vertex_pos;

        while (!stream.EndOfStream()) {
            for (uint32_t i{0}; i < 3; i++) {
                AX_PROPAGATE_RESULT_ERROR(stream.Read(&vertex_pos[i], sizeof(float)));
            }
            if (p.unset) {
                p.min = p.max = vertex_pos;
                p.unset = false;
            }
            p.min = glm::min(p.min, vertex_pos);
            p.max = glm::max(p.max, vertex_pos);
        }
    }
    for (auto child : p.node.children) {
        GetMinMax({p.nodeInstance, p.node, p.min, p.max, p.unset});
    }
}

utils::ExError NodeInstance::Handle(const NodeHandleParams& p) {
    for (auto& nMesh : rootNode.meshes) {
        this->meshes.push_back(nMesh);
    }
    for (auto& child : rootNode.children) {
        MeshInstance *childM = new MeshInstance(child);
        this->children.push_back(childM);
    }
    this->name = rootNode.name;
    this->decompose(rootNode.transform);
    
    bool unset = true;
    glm::vec3 min(0.0f), max(0.0f);
    // getMinMax(rootNode, min, max, unset);
    // this->bounding = AABB(min, max);

    m_MeshIds = p.node.meshIds;
    m_Coords = {p.node.transform};
    for (const auto& child : p.node.children) {
        Handle(p);
    }
}

NodeInstance::NodeInstance(const NodeInstanceParams& p) : m_Params(p) {
    const auto& root = GetRoot();
    glm::vec3 min, max;
    bool unset{true};
    Handle({*this, root, min, max, unset});
}

NodeInstance::~NodeInstance() {
    m_MeshIds.clear();
    m_Children.clear();
}

const assets::Node& NodeInstance::GetRoot() const {
    return m_Params.immutableImport.nodes[m_Params.assetNodeIdx];
}

bool NodeInstance::IsDirty() {
    std::lock_guard<std::mutex> mutex(m_Mutex);
    return m_Dirty;
}

void NodeInstance::ApplyCoords(const std::function<void(utils::Coordination&)>& consumer) {
    std::lock_guard<std::mutex> mutex(m_Mutex);
    consumer(m_Coords);
    m_Dirty = true;
}

utils::Coordination NodeInstance::GetCoords() {
    std::lock_guard<std::mutex> mutex(m_Mutex);
    return m_Coords;
}

utils::Coordination NodeInstance::UseCoords() {
    std::lock_guard<std::mutex> mutex(m_Mutex);
    m_Dirty = false;
    return m_Coords;
}

const std::vector<NodeInstance>& NodeInstance::GetChildren() {
    return m_Children;
}

const std::vector<uint32_t>& NodeInstance::GetMeshIds() {
    return m_MeshIds;
}

std::string_view NodeInstance::GetName() const {
    return GetRoot().name;
}

}

