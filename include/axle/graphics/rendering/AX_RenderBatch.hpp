#pragma once

#include "axle/graphics/AX_GraphicsParams.hpp"
#include "axle/graphics/rendering/AX_RenderLayer.hpp"

#include "axle/utils/AX_Types.hpp"
#include "axle/utils/AX_Expected.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <type_traits>

namespace axle::gfx
{

enum class MeshMode : uint8_t {
    Vertices,
    Indexed
};

struct DrawItem {
    MeshMode meshMode;

    RenderPipelineHandle pipeline;

    BufferHandle vertices;
    BufferHandle indices;

    ResourceSetHandle resources;

    uint32_t vertexCount, indexCount;
    uint32_t firstVertex{0}, firstIndex{0};

    uint32_t sortKey;
};

const inline Predicate<DrawItem> RPROC_SORT_BY_WEIGHT = [](const DrawItem& a, const DrawItem& b) {
    return a.sortKey > b.sortKey;
};

using BatchErrorPredicate = std::function<bool(const utils::ExError&)>;

struct RenderBatchDesc {
    SharedPtr<ICommandList> commandList;
    BatchErrorPredicate frameErrorHandler;
};

class RenderBatch {
private:
    std::deque<DrawItem> m_Items{};

    std::unordered_map<RLHandle, SharedPtr<RenderLayer>> m_Registries{};
    std::unordered_map<SharedPtr<RenderLayer>, std::vector<RLHandle>> m_RegistriesRev{};

    SharedPtr<ICommandList> m_RenderCmds;

    std::mutex m_Mutex{};
public:
    RenderBatch(const RenderBatchDesc& desc);
    ~RenderBatch();

    void ResetItems();

    void Submit(const DrawItem& item);
    void Submit(utils::Span<DrawItem> itemsView);

    void ResetAndSubmit(const DrawItem& item);
    void ResetAndSubmit(utils::Span<DrawItem> itemsView);

    utils::ExResult<RLHandle> Register(SharedPtr<RenderLayer> rl, const RLStage& stage, uint32_t sortKey = 0);
    utils::ExError UnRegister(const RLHandle& handle);

    void Sort(const Predicate<DrawItem>& pred = RPROC_SORT_BY_WEIGHT);
protected:
    friend RenderLayer;

    BatchErrorPredicate m_ErrorHandler{};

    static void Draw(SharedPtr<core::ThreadContextGfx> thrCtx, float dT, void* userPtr);
};

}