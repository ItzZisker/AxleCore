#pragma once

#include "axle/assets/AX_AssetImporter.hpp"
#include "axle/assets/AX_AssetGpu.hpp"

#include "axle/core/concurrency/AX_ThreadCycler.hpp"

#include "axle/graphics/scene/AX_SWFrustumCulling.hpp"

#include "axle/utils/AX_Coordination.hpp"
#include "axle/utils/AX_Types.hpp"

namespace axle::gfx { class RenderBatch; }
namespace axle::scene { class ModelInstance; }

namespace axle::scene
{

struct NodeInstanceParams {
    const assets::AssetImportResult& immutableImport;
    const uint32_t assetNodeIdx;
};

// RenderThread Owns This
class NodeInstance : AX_THR_RENDER_OWNED {
private:
    NodeInstanceParams m_Params;

    std::vector<NodeInstance> m_Children{};
    std::vector<uint32_t> m_MeshIds{};
    std::string m_Name{};

    bool m_Dirty{true};
    
    typedef struct {
        NodeInstance& nodeInstance;
        const assets::Node& node;
        glm::vec3 &min;
        glm::vec3 &max;
        bool &unset;
    } NodeHandleParams;

    utils::ExError GetMinMax(const NodeHandleParams& params);
    utils::ExError Handle(const NodeHandleParams& params);
public:
    ~NodeInstance();

    NodeInstance(const NodeInstance&) = delete;
    NodeInstance& operator=(const NodeInstance&) = delete;

    const assets::NodeId GetId() const;
    const assets::Node& GetRoot() const;

    ThreadInvocation<bool> IsDirty();

    ThreadInvocationVoid SetCoords(const utils::Coordination& coords);
    ThreadInvocation<utils::Coordination> GetCoords();
    ThreadInvocation<utils::Coordination> UseCoords();

    utils::Span<uint32_t> GetMeshIds();
    utils::Span<NodeInstance> GetChildren();

    std::string_view GetName() const;
protected:
    utils::Coordination m_Coords;

    explicit NodeInstance(ThreadGfxScope gfxThread, const NodeInstanceParams& params);
};

}