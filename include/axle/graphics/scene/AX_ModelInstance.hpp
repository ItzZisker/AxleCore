#pragma once

#include "axle/utils/AX_Coordination.hpp"

namespace axle::scene
{

// TODO: SWDiscardable

class ModelInstance {
private:
    Model* model;

    NodeInstance* root;
    std::unordered_map<NodeInstance*, std::vector<NodeInstance*>> leafParents;
    std::unordered_map<NodeInstance*, glm::mat4> cachedLeafFinalTransform;

    std::set<NodeInstance*> discarded;

public:
    ModelInstance(Model* model);

    void PushLeafParents(NodeInstance *meI, std::vector<NodeInstance*> parentList);

    void ClearCachedTransforms();
    void ClearLeafParents();
    void ClearDiscarded();

    glm::mat4 GetWorldTransform(NodeInstance* leaf, bool cacheFinalTransform = true);
    glm::mat4 GetCachedWorldTransform(NodeInstance *leaf);

    bool IsTransformCached(NodeInstance *leaf);

    void SetDiscard(NodeInstance* meI, bool discard);

    bool DiscardOrNot(NodeInstance* meI);
    bool DiscardOrNot(Scene_T snapshot, const glm::mat4& transform) override;

    NodeInstance* GetRoot();
    Model* GetModel();
};

}