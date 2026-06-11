#pragma once

#include "axle/graphics/AX_GraphicsParams.hpp"
#include "axle/graphics/rendering/AX_RenderLayer.hpp"

#include "axle/utils/AX_Types.hpp"
#include "axle/utils/AX_Expected.hpp"

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

    RenderPipelineHandle pipeline; // Pipeline (shader, geometry, blending states, etc.)

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
    SharedPtr<core::ThreadContextGfx> gfxThread{nullptr};
    SharedPtr<ICommandList> commandList{nullptr};
    
    BatchErrorPredicate frameErrorHandler{RBATCH_DEFAULT_ERROR_HANDLER};
    Predicate<DrawItem> itemsSort{RBATCH_SORT_BY_MINIMAL_STATE};
    UserSortKeyAssigner userSortKeyAssigner{RBATCH_DEFAULT_USER_SORTKEY_ZERO};
    
    RenderBatchType batchType{RenderBatchType::DirectDraw};
    TransformInput transformInput{TransformInput::VertexInput};
    
    bool separateSamplersFromTextures{false}; // Requires FeatureSet on Host device
    bool autoSort{false}; // Sorts on run (Add/Remove/Modify Instances)
};

struct ModelInstanceTag {};
struct ModelInstanceHandle : public utils::MagicHandleTagged<ModelInstanceTag> {};

struct ModelInstanceWrapper : public utils::MagicInternal<ModelInstanceHandle> {
    SharedPtr<scene::ModelInstance> value;
};

class RenderBatch {
private:
    utils::MagicPool<ModelInstanceWrapper> m_Instances{};

    std::deque<DrawItem> m_Items{};
    std::unordered_set<SharedPtr<scene::ModelInstance>> m_TrackingInstances{};

    std::unordered_map<RLHandle, SharedPtr<RenderLayer>> m_Registries{};
    std::unordered_map<SharedPtr<RenderLayer>, std::vector<RLHandle>> m_RegistriesRev{};

    std::mutex m_Mutex{};

    void TraverseNode(
        scene::ModelInstance& modelInstance,
        scene::NodeInstance& nodeInstance,
        std::unordered_set<DrawItem>& results
    );

    void AddInstance0(SharedPtr<scene::ModelInstance> modelInstance);
    void RebuildDrawItems();
public:
    RenderBatch(const RenderBatchDesc& desc);
    ~RenderBatch();

    void Clear();

    void AddInstance(SharedPtr<scene::ModelInstance> modelInstance);
    void AddInstances(utils::Span<SharedPtr<scene::ModelInstance>> modelInstances);

    utils::ExResult<RLHandle> Register(SharedPtr<RenderLayer> rl, const RLStage& stage, uint32_t sortKey = 0);
    utils::ExError UnRegister(const RLHandle& handle);

    void SetItemSort(const Predicate<DrawItem>& pred);
    void SetUserSortKeyAssigner(const UserSortKeyAssigner& assigner);
    void SortItems();
protected:
    friend RenderLayer;

    RenderBatchDesc m_Desc{};

    static void Draw(SharedPtr<core::ThreadContextGfx> thrCtx, float dT, void* userPtr);
};

}