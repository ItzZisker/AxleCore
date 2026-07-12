#pragma once

#include "axle/assets/AX_AssetGpu.hpp"
#include "axle/assets/AX_AssetImporter.hpp"

#include "axle/graphics/rendering/AX_RenderProcedureShader.hpp"
#include "axle/graphics/scene/AX_NodeInstance.hpp"

#include "axle/utils/AX_Coordination.hpp"

#include <unordered_set>

namespace axle::gfx { class RPPipelineResolver; }

namespace axle::scene
{

struct MeshBinds {
    std::unordered_map<assets::MeshId, assets::AssetGpuMesh> draw;
    std::unordered_map<assets::MeshId, assets::AssetGpuMaterial> materials; 
};

struct MaterialStateMesh {
    utils::UUID shaderId;

    gfx::RasterState raster{};
    gfx::DepthStencilState depth{};
    gfx::BlendState blend{};
};

struct MaterialStateMeshKeyed {
    assets::MeshId meshId;
    MaterialStateMesh state;    
};

struct MaterialState {
    utils::CowSpan<MaterialStateMeshKeyed> meshStates;
    utils::CowSpan<assets::NodeId> discards;

    uint32_t fallbackMeshStateIdx{0};
};

struct ModelDesc {
    ThreadGfxScope gfxThread;
    const assets::AssetImportResult& immutableImport;
    SharedPtr<gfx::RPShaderManager> shaderProcMgr;
    assets::Node rootNode;
    MaterialState materialState;
    MeshBinds meshBinds;
};

class ModelInstance : AX_THR_RENDER_OWNED {
private:
    const ModelDesc m_Desc;
    utils::Coordination m_Coords;

    std::unordered_map<assets::NodeId, SharedPtr<scene::NodeInstance>> m_NodeInstancesById;
    std::unordered_map<assets::NodeId, std::vector<assets::NodeId>> m_LeafParents;
    std::unordered_map<assets::NodeId, glm::mat4> m_CachedLeafFinalTransform;

    SharedPtr<NodeInstance> m_RootNodeInstance;

    void SetDiscard0(assets::NodeId nodeId, bool discard);
    void SetMeshState0(assets::MeshId meshId, const MaterialStateMesh& matState);

    void TraverseNode(assets::Node root);
protected:
    std::unordered_map<assets::MeshId, MaterialStateMesh> m_MeshStates;
    std::unordered_set<assets::NodeId> m_Discards;
    MaterialStateMesh m_FallbackMeshState{};

    friend class gfx::RPPipelineResolver;
public:
    explicit ModelInstance(const ModelDesc& desc);

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

    ThreadInvocationVoid SetMeshState(assets::MeshId meshId, const MaterialStateMesh& matState);
    ThreadInvocationVoid SetMeshesStates(const std::unordered_map<assets::MeshId, MaterialStateMesh>& map);
    
    ThreadInvocationVoid SetDiscard(assets::NodeId nodeInstance, bool discard);
    ThreadInvocationVoid SetDiscards(const std::unordered_map<assets::NodeId, bool>& map);

    ThreadInvocationVoid SetFallbackMeshState(const MaterialStateMesh& matState);

    // TODO: Software frustum discardable in future
    // bool DiscardOrNot(assets::NodeId nodeInstance);
    // bool DiscardOrNot(Scene_T snapshot, const glm::mat4& transform) override;
};

}