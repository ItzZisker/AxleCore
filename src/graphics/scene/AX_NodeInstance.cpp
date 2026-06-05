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

NodeInstance::NodeInstance(const NodeInstanceParams& params) : m_Params(params) {}
NodeInstance::~NodeInstance() {

}

const assets::Node& NodeInstance::GetRoot() {
    return m_Params.immutableImport.nodes[m_Params.assetNodeIdx];
}

bool NodeInstance::IsDirty() const {
    return m_Dirty;
}

void NodeInstance::MarkDirty(bool dirty) {
    m_Dirty = dirty;
}

}

