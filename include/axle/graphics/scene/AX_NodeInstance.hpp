#pragma once

#include "axle/assets/AX_AssetImporter.hpp"
#include "axle/assets/AX_AssetGpu.hpp"

#include "axle/graphics/scene/AX_SWFrustumCulling.hpp"

#include "axle/utils/AX_Coordination.hpp"
#include "axle/utils/AX_Types.hpp"

namespace axle::gfx { class RenderBatch; }

namespace axle::scene
{

struct NodeInstanceParams {
    const assets::AssetImportResult& immutableImport;
    uint32_t assetNodeIdx;
};

// RenderThread Owns This

// TODO: NodeInstance : public SWFrustumDiscardable
class NodeInstance {
private:
    NodeInstanceParams m_Params;

    utils::Coordination m_Coords;

    std::vector<NodeInstance> m_Children{};
    std::vector<uint32_t> m_MeshIds{};
    std::string m_Name{};

    bool m_Dirty{true};
    std::mutex m_Mutex{};
    
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
    NodeInstance(const NodeInstanceParams& params);
    ~NodeInstance();

    const assets::Node& GetRoot() const;

    bool IsDirty();

    void ApplyCoords(const std::function<void(utils::Coordination&)>& consumer);
    utils::Coordination GetCoords();

    const std::vector<uint32_t>& GetMeshIds();
    const std::vector<NodeInstance>& GetChildren();

    std::string_view GetName() const;
protected:
    friend gfx::RenderBatch;

    utils::Coordination UseCoords();
};

}