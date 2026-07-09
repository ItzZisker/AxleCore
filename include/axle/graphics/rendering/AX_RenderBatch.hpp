#pragma once

#include "axle/graphics/AX_GraphicsParams.hpp"
#include "axle/graphics/rendering/AX_RenderLayer.hpp"

#include "axle/utils/AX_Types.hpp"
#include "axle/utils/AX_Expected.hpp"
#include "axle/utils/AX_UUID.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <type_traits>
#include <unordered_set>
#include <iostream>

namespace axle::scene { class ModelInstance; }
namespace axle::core { class ThreadContextGfx; }

namespace axle::gfx
{

enum class MeshMode : uint8_t {
    Vertices,
    Indexed
};

struct DrawItem {
    MeshMode meshMode; // Raw vertices, Indexed vertices

    RenderPipelineHandle pipeline{utils::INVALID_HANDLE}; // Pipeline (shader, geometry, blending states, etc.)

    BufferHandle vertices; // Mesh
    BufferHandle indices; // Mesh

    ResourceSetHandle resources; // Material + Additonal resources

    uint32_t vertexCount, indexCount; // DrawCall Params
    uint32_t firstVertex{0}, firstIndex{0}; // DrawCall Params

    uint32_t sortKey{0}; // User-defined sortKey (Optional)
};

const inline Predicate<DrawItem> RBATCH_SORT_BY_MINIMAL_STATE = [](const DrawItem& a, const DrawItem& b) {
    if (a.pipeline != b.pipeline) return a.pipeline < b.pipeline;
    if (a.resources != b.resources) return a.resources < b.resources;
    if (a.vertices != b.vertices) return a.vertices < b.vertices;
    if (a.indices != b.indices) return a.indices < b.indices;
    if (a.meshMode != b.meshMode) return a.meshMode < b.meshMode;

    // Optional tie-breakers for deterministic ordering
    if (a.firstVertex != b.firstVertex) return a.firstVertex < b.firstVertex;
    if (a.firstIndex != b.firstIndex) return a.firstIndex < b.firstIndex;
    if (a.vertexCount != b.vertexCount) return a.vertexCount < b.vertexCount;
    if (a.indexCount != b.indexCount) return a.indexCount < b.indexCount;

    return a.sortKey < b.sortKey;
};

using BatchErrorPredicate = std::function<bool(const utils::ExError&)>;

const inline BatchErrorPredicate RBATCH_DEFAULT_ERROR_HANDLER = [](const utils::ExError& error) {
    if (error.IsValid()) {
        std::cerr << "Frame handler Error: " << error.GetMessage() << std::endl;
        return false;
    } else {
        return true;
    }
};

enum class RenderBatchType {
    DirectDraw,
    DirectInstancedDraw,
    IndirectDraw
};

enum class TransformInput {
    VertexInput,
    Uniforms
};

struct UserSortKeyParams {
    scene::ModelInstance& modelInstance;
    scene::NodeInstance& nodeInstance;
    uint32_t meshId;
};

using UserSortKeyAssigner = std::function<uint32_t(const UserSortKeyParams&)>;

const inline UserSortKeyAssigner RBATCH_DEFAULT_USER_SORTKEY_ZERO = [](const UserSortKeyParams&) {
    return 0u;
};

struct RenderBatchDesc {
    Predicate<DrawItem> itemsSort{RBATCH_SORT_BY_MINIMAL_STATE};
    UserSortKeyAssigner userSortKeyAssigner{RBATCH_DEFAULT_USER_SORTKEY_ZERO};
    
    RenderBatchType batchType{RenderBatchType::DirectDraw};
    TransformInput transformInput{TransformInput::VertexInput};
    
    bool separateSamplersFromTextures{false}; // Requires FeatureSet on Host device
};

struct NodeTraversalParams {
    scene::ModelInstance& modelInstance;
    scene::NodeInstance& nodeInstance;
    std::deque<DrawItem>& results;
};

class RenderProcedure;

class RenderBatch : AX_THR_RENDER_OWNED {
private:
    std::unordered_set<SharedPtr<scene::ModelInstance>> m_TrackingInstances{};

    void TraverseNode(const NodeTraversalParams& params);

    void AddInstance0(SharedPtr<scene::ModelInstance> modelInstance);
    void RemoveInstance0(SharedPtr<scene::ModelInstance> modelInstance);

    void GenerateDrawCalls(SharedPtr<scene::ModelInstance> modelInstance, std::deque<DrawItem>& out);
public:
    RenderBatch(ThreadGfxScope gfxThread, const RenderBatchDesc& desc);

    ThreadInvocationVoid ClearInstances();

    ThreadInvocationVoid AddInstance(SharedPtr<scene::ModelInstance> modelInstance);
    ThreadInvocationVoid AddInstances(utils::Span<SharedPtr<scene::ModelInstance>> modelInstances);

    ThreadInvocationVoid RemoveInstance(SharedPtr<scene::ModelInstance> modelInstance);
    ThreadInvocationVoid RemoveInstances(utils::Span<SharedPtr<scene::ModelInstance>> modelInstances);

    ThreadInvocationVoid SetItemSort(const Predicate<DrawItem>& pred);
    ThreadInvocationVoid SetUserSortKeyAssigner(const UserSortKeyAssigner& assigner);
protected:
    friend class RenderProcedure;

    RenderBatchDesc m_Desc{};

    void Record(const RenderProcedure& renderProc);
};

}