#pragma once

#include "axle/graphics/AX_GraphicsParams.hpp"
#include "axle/graphics/layer/AX_RenderLayer.hpp"

#include "axle/utils/AX_Types.hpp"
#include "axle/utils/AX_Expected.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <type_traits>

namespace axle::gfx
{

enum class RenderItemType : uint8_t {
    Vertices,
    Indexed
};

struct RenderItem {
    RenderItemType type;

    RenderPipelineHandle pipeline;

    BufferHandle vertices;
    BufferHandle indices;

    ResourceSetHandle resources;

    uint32_t vertexCount, indexCount;
    uint32_t firstVertex{0}, firstIndex{0};

    uint32_t sortKey;
};

const inline Predicate<RenderItem> RPROC_SORT_BY_WEIGHT = [](const RenderItem& a, const RenderItem& b) {
    return a.sortKey > b.sortKey;
};

class RenderProcedure {
private:
    std::deque<RenderItem> m_Items{};

    std::unordered_map<RLHandle, SharedPtr<RenderLayer>> m_Registries{};
    std::unordered_map<SharedPtr<RenderLayer>, std::vector<RLHandle>> m_RegistriesRev{};

    SharedPtr<ICommandList> m_RenderCmds;

    std::mutex m_Mutex{};
public:
    RenderProcedure(SharedPtr<ICommandList> renderCmds);
    ~RenderProcedure();

    void ResetItems();

    void Submit(const RenderItem& item);
    void Submit(utils::Span<RenderItem> itemsView);

    void ResetAndSubmit(const RenderItem& item);
    void ResetAndSubmit(utils::Span<RenderItem> itemsView);

    utils::ExResult<RLHandle> Register(SharedPtr<RenderLayer> rl, const RLStage& stage, uint32_t sortKey = 0);
    utils::ExError UnRegister(const RLHandle& handle);

    void Sort(const Predicate<RenderItem>& pred = RPROC_SORT_BY_WEIGHT);
protected:
    friend RenderLayer;

    static void Draw(SharedPtr<core::ThreadContextGfx> thrCtx, float dT, void* userPtr);
};

}