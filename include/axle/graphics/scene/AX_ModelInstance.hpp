#pragma once

#include "axle/assets/AX_AssetGpu.hpp"
#include "axle/assets/AX_AssetImporter.hpp"

#include "axle/graphics/scene/AX_NodeInstance.hpp"

#include "axle/utils/AX_Coordination.hpp"

#include <unordered_set>

namespace axle::scene
{

struct MeshBinds {
    std::unordered_map<assets::MeshId, gfx::RenderPipelineHandle> pipeline;

    std::unordered_map<assets::MeshId, assets::AssetGpuMesh> draw;
    std::unordered_map<assets::MeshId, assets::AssetGpuMaterial> materials; 
};

struct ModelDesc {
    const assets::AssetImportResult& immutableImport;
    assets::Node rootNode;
    MeshBinds meshBinds;
};

// RenderThread Owns This
class ModelInstance : AX_THR_RENDER_OWNED {
private:
    const ModelDesc m_Desc;
    utils::Coordination m_Coords;

    std::unordered_map<assets::NodeId, SharedPtr<scene::NodeInstance>> m_NodeInstancesById;
    std::unordered_map<assets::NodeId, std::vector<assets::NodeId>> m_LeafParents;
    std::unordered_map<assets::NodeId, glm::mat4> m_CachedLeafFinalTransform;

    std::unordered_set<assets::NodeId> m_Discards;

    SharedPtr<NodeInstance> m_RootNodeInstance;
protected:
    void TraverseNode(assets::Node root);

    explicit ModelInstance(ThreadGfxScope gfxThread, const ModelDesc& desc);
public:
    const ModelDesc& GetModelDesc() const;

    ThreadInvocation<SharedPtr<NodeInstance>> GetNode(assets::NodeId id) const;
    ThreadInvocation<SharedPtr<NodeInstance>> GetRootNode() const;

    ThreadInvocationVoid PushLeafParents(assets::NodeId, std::vector<assets::NodeId> parentList);

    ThreadInvocationVoid ClearCachedTransforms();
    ThreadInvocationVoid ClearLeafParents();
    ThreadInvocationVoid ClearDiscarded();

    ThreadInvocation<glm::mat4> GetWorldTransform(assets::NodeId leafInstance, bool cacheFinalTransform = true);
    ThreadInvocation<glm::mat4> GetCachedWorldTransform(assets::NodeId leafInstance);

    ThreadInvocation<bool> IsTransformCached(assets::NodeId leafInstance);

    ThreadInvocationVoid SetDiscard(assets::NodeId nodeInstance, bool discard);

    // TODO: Software frustum discardable in future
    // bool DiscardOrNot(assets::NodeId nodeInstance);
    // bool DiscardOrNot(Scene_T snapshot, const glm::mat4& transform) override;
};

}