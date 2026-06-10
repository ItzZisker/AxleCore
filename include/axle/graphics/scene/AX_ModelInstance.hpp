#pragma once

#include "axle/assets/AX_AssetGpu.hpp"
#include "axle/assets/AX_AssetImporter.hpp"

#include "axle/graphics/scene/AX_NodeInstance.hpp"

#include "axle/utils/AX_Coordination.hpp"

#include <unordered_set>

namespace axle::scene
{

struct MeshBounds {
    std::unordered_map<assets::MeshId, utils::CowSpan<assets::AssetGpuMesh>> draw;
    std::unordered_map<assets::MeshId, utils::CowSpan<assets::AssetGpuMaterial>> materials; 
};

struct ModelDesc {
    MeshBounds bounds;
    assets::Node root;
};

// RenderThread Owns This

// TODO: NodeInstance : public SWDiscardable
class ModelInstance {
private:
    ModelDesc m_Desc;
    utils::Coordination m_Coords;

    std::unordered_map<assets::NodeId, std::vector<assets::NodeId>> m_LeafParents;
    std::unordered_map<assets::NodeId, glm::mat4> m_CachedLeafFinalTransform;

    std::unordered_set<assets::NodeId> m_Discards;
protected:
    explicit ModelInstance(const ModelDesc& desc);
public:
    void PushLeafParents(assets::NodeId, std::vector<assets::NodeId> parentList);

    void ClearCachedTransforms();
    void ClearLeafParents();
    void ClearDiscarded();

    glm::mat4 GetWorldTransform(assets::NodeId leafInstance, bool cacheFinalTransform = true);
    glm::mat4 GetCachedWorldTransform(assets::NodeId leafInstance);

    bool IsTransformCached(assets::NodeId leafInstance);

    void SetDiscard(assets::NodeId nodeInstance, bool discard);

    // bool DiscardOrNot(assets::NodeId nodeInstance);
    // bool DiscardOrNot(Scene_T snapshot, const glm::mat4& transform) override;

    const NodeInstance& GetRoot() const;
    const ModelDesc& GetModelDesc() const;
};

}