#pragma once

#include "axle/core/concurrency/AX_ThreadCycler.hpp"

#include "axle/utils/AX_MagicPool.hpp"
#include "axle/utils/AX_Expected.hpp"
#include "axle/utils/AX_Types.hpp"

#include <vector>

namespace axle::gfx
{

enum class RLStage : uint8_t {
    Early,    // e.g. clear, global setup, shadow passes
    Normal,   // most stuff
    Late,     // overlays, UI, debug
};

using RLFunc = std::function<void(core::ThreadContextGfx* thrCtx, float dT, void* userPtr)>;
using RLOrderKey = utils::MagicId;

struct RLHandle : public utils::MagicHandle {};

struct RLDesc {
    RLFunc      updateFunc{nullptr};
    RLFunc      drawFunc{nullptr};
    RLStage     stage{RLStage::Normal};
    RLOrderKey  sortKey{0};
};

struct RLIntern : public utils::MagicInternal<RLHandle> {
    RLFunc      updateFunc{nullptr};
    RLFunc      drawFunc{nullptr};
    RLStage     stage{RLStage::Normal};
    RLOrderKey  sortKey{0};
};

struct RLRegistry {
    FramebufferHandle framebuffer{};
    core::WorkHandle workHandle{};
    SharedPtr<core::ThreadContextGfx> gfxThread{nullptr};
    
    bool active{false};
};

const inline Predicate<RLIntern> SORT_BY_WEIGHT = [](const RLIntern& a, const RLIntern& b){
    return a.sortKey > b.sortKey;
};

class RenderLayer {
private:
    RLRegistry m_RLRegistry{};

    utils::MagicPool<RLIntern> m_RLPool{utils::MagicPool<RLIntern>(true)};
    SharedPtr<void> m_shUserPtr{nullptr};
    void *m_UserPtr{nullptr};

    std::mutex m_Mutex{};

    void Update(float dT);
    void Draw(float dT);
public:
    RenderLayer(void* userPtr = nullptr);
    RenderLayer(SharedPtr<void> userPtr = nullptr);
    ~RenderLayer();

    utils::ExResult<RLHandle> CreateLayer(const RLDesc& desc);
    utils::ExError RemoveLayer(const RLHandle& handle);

    void SortLayers(const Predicate<RLIntern>& pred = SORT_BY_WEIGHT);

    const utils::ExResult<RLRegistry> GetCurrentRegistry();

    utils::ExError RegisterWork(FramebufferHandle fb, SharedPtr<core::ThreadContextGfx> thrCtx);
    utils::ExError UnRegisterWork();
};

}