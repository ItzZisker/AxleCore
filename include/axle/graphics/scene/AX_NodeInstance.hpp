#pragma once

#include "axle/graphics/scene/AX_SWFrustumCulling.hpp"
#include "axle/utils/AX_Coordination.hpp"

namespace axle::scene
{

class NodeInstance : public SWFrustumDiscardable {
private:
    std::string name = "NONE";

    std::vector<NodeInstance*> children = {};
    std::vector<NamedMesh*> meshes = {};

    bool markedDirty = false;

    void Handle(LocalNode& node, glm::vec3& min, glm::vec3& max, bool& unset);
public:
    NodeInstance(NamedMesh* namedMesh);
    NodeInstance(NamedMesh* namedMesh, Coordination coords);
    NodeInstance(LocalNode rootNode);
    ~NodeInstance();

    void MarkDirty(bool flag);
    bool IsDirty();

    void UpdateTransform() override;

    std::vector<NodeInstance*>& GetChildren();
    std::vector<NamedMesh*>& GetMeshes();

    const std::string& GetName();
    bool HasMeshes();
};

}