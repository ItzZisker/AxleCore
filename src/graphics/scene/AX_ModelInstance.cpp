#include "axle/graphics/scene/AX_ModelInstance.hpp"

namespace axle::scene
{

void ModelInstance::TraverseNode(assets::Node root) {
    NodeInstanceParams params = {m_Desc.immutableImport, root.nodeId};
    m_NodeInstancesById[root.nodeId] = std::make_shared<NodeInstance>(params);
    for (auto& child : root.children) TraverseNode(child);
}

ThreadInvocation<SharedPtr<NodeInstance>> ModelInstance::GetNode(assets::NodeId id) const {
    return ThreadInvocation<SharedPtr<NodeInstance>>(m_Thread, [&](){
        auto it = m_NodeInstancesById.find(id);
        if (it == m_NodeInstancesById.end()) {
            return SharedPtr<NodeInstance>(nullptr);
        } else {
            return it->second;
        }
    });
}

ModelInstance::ModelInstance(const ModelDesc& desc) : ThreadOwned(desc.gfxThread), m_Desc(desc) {
    TraverseNode(desc.rootNode);
    m_RootNodeInstance = m_NodeInstancesById[desc.rootNode.nodeId];
}

ThreadInvocationVoid ModelInstance::PushLeafParents(assets::NodeId nodeId, std::vector<assets::NodeId> parentList) {
    return ThreadInvocationVoid(m_Thread, [&](){
        auto itNode = m_NodeInstancesById.find(nodeId);
        if (itNode == m_NodeInstancesById.end()) 
            return VoidInvoke{};

        auto node = itNode->second;
        if (node->GetMeshIds().size() > 0) {
            m_LeafParents.insert({nodeId, parentList});
        }
        for (auto& child : node->GetChildren()) {
            auto childId = child.GetId();

            parentList.push_back(childId);
            PushLeafParents(childId, parentList);
            parentList.pop_back();
        }
        return VoidInvoke{};
    });
}

ThreadInvocationVoid ModelInstance::ClearCachedTransforms() {
    return ThreadInvocationVoid(m_Thread, [&](){
        m_CachedLeafFinalTransform.clear();
        return VoidInvoke{};
    });
}

ThreadInvocationVoid ModelInstance::ClearLeafParents() {
    return ThreadInvocationVoid(m_Thread, [&](){
        m_LeafParents.clear();
        return VoidInvoke{};
    });
}

ThreadInvocationVoid ModelInstance::ClearDiscarded() {
    return ThreadInvocationVoid(m_Thread, [&](){
        m_Discards.clear();
        return VoidInvoke{};
    });
}

ThreadInvocation<glm::mat4> ModelInstance::GetWorldTransform(assets::NodeId leafId, bool cacheFinalTransform = true) {
    return ThreadInvocation<glm::mat4>(m_Thread, [&](){
        auto it = m_NodeInstancesById.find(leafId);
        if (it == m_NodeInstancesById.end()) {
            return glm::mat4(1.0f);
        }

        auto leaf = it->second;
        if (!leaf) return glm::mat4(1.0f);

        auto itCache = m_CachedLeafFinalTransform.find(leafId);
        auto isResLeafDirty = leaf->IsDirty().Call();

        if (!isResLeafDirty.has_value()) {
            return glm::mat4(1.0f);
        }
        auto isLeafDirty = isResLeafDirty.value();

        if (itCache != m_CachedLeafFinalTransform.end() && !isLeafDirty) {
            return itCache->second;
        }

        glm::mat4 world = m_Coords.GetTransform();
        auto& ancestry = m_LeafParents[leafId];

        int i{0};
        for (auto nodeId : ancestry) {
            auto it = m_NodeInstancesById.find(leafId);
            if (it == m_NodeInstancesById.end()) {
                return glm::mat4(1.0f);
            }
            auto node = it->second;
            world *= node->UseCoords().SyncCall();
        }

        world *= leaf->UseCoords().SyncCall();

        if (cacheFinalTransform) {
            m_CachedLeafFinalTransform.emplace(leafId, world);
        }
        return world;
    });
}

ThreadInvocation<glm::mat4> ModelInstance::GetCachedWorldTransform(assets::NodeId leafId) {
    return ThreadInvocation<glm::mat4>(m_Thread, [&](){
        if (m_CachedLeafFinalTransform.find(leafId) != m_CachedLeafFinalTransform.end()) {
            return m_CachedLeafFinalTransform[leafId];
        } else {
            return glm::mat4(1.0f);
        }
    });
}

ThreadInvocation<bool> ModelInstance::IsTransformCached(assets::NodeId leafId) {
    return ThreadInvocation<bool>(m_Thread, [&](){
        return m_CachedLeafFinalTransform.find(leafId) != m_CachedLeafFinalTransform.end();
    });
}

void ModelInstance::SetDiscard0(assets::NodeId nodeId, bool discard) {
    auto it = m_Discards.find(nodeId);
    if (!discard && it != m_Discards.end()) {
        m_Discards.erase(it);
    } else if (discard && it == m_Discards.end()) {
        m_Discards.insert(nodeId);
    }
}

ThreadInvocationVoid ModelInstance::SetDiscard(assets::NodeId nodeId, bool discard) {
    return ThreadInvocationVoid(m_Thread, [&](){
        ModelInstance::SetDiscard0(nodeId, discard);
        return VoidInvoke{};
    });
}

ThreadInvocationVoid ModelInstance::SetDiscards(const std::unordered_map<assets::NodeId, bool>& map) {
    return ThreadInvocationVoid(m_Thread, [&](){
        for (const auto& [nodeId, discard] : map)
            ModelInstance::SetDiscard0(nodeId, discard);
        return VoidInvoke{};
    });
}

void ModelInstance::SetMeshState0(assets::MeshId meshId, const MaterialStateMesh& matState) {
    m_MeshStates[meshId] = matState;
}

ThreadInvocationVoid ModelInstance::SetMeshState(assets::MeshId meshId, const MaterialStateMesh& matState) {
    return ThreadInvocationVoid(m_Thread, [&](){
        ModelInstance::SetMeshState0(meshId, matState);
        return VoidInvoke{};
    });
}

ThreadInvocationVoid ModelInstance::SetMeshesStates(const std::unordered_map<assets::MeshId, MaterialStateMesh>& map) {
    return ThreadInvocationVoid(m_Thread, [&](){
        for (const auto& [meshId, meshState] : map)
            ModelInstance::SetMeshState0(meshId, meshState);
        return VoidInvoke{};
    });
}

ThreadInvocationVoid ModelInstance::SetFallbackMeshState(const MaterialStateMesh& matState) {
    return ThreadInvocationVoid(m_Thread, [&](){
        m_FallbackMeshState = matState;
        return VoidInvoke{};
    });
}

ThreadInvocation<SharedPtr<NodeInstance>> ModelInstance::GetRootNode() const {
    return GetNode(m_Desc.rootNode.nodeId);
}

const ModelDesc& ModelInstance::GetModelDesc() const {
    return m_Desc;
}

}