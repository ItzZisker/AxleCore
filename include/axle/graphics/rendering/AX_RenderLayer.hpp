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

using RLFunc = std::function<void(SharedPtr<core::ThreadContextGfx> thrCtx, float dT, void* userPtr)>;
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
    bool active{false};
};

const inline Predicate<RLIntern> RL_SORT_BY_WEIGHT = [](const RLIntern& a, const RLIntern& b){
    return a.sortKey > b.sortKey;
};

class RenderLayer : AX_THR_RENDER_OWNED {
private:
    RLRegistry m_RLRegistry{};

    utils::MagicPool<RLIntern> m_RLPool{utils::MagicPool<RLIntern>(true)};
    SharedPtr<void> m_shUserPtr{nullptr};
    void *m_UserPtr{nullptr};
    
    // These will run on the render thread, so becareful with your user data!
    void Update(float dT);
    void Draw(float dT); 
public:
    RenderLayer(ThreadGfxScope gfxThread, void* userPtr = nullptr);
    RenderLayer(ThreadGfxScope gfxThread, SharedPtr<void> userPtr = nullptr);
    ~RenderLayer();

    AX_NON_COPYABLE_NON_MOVABLE(RenderLayer);

    ThreadInvocation<RLHandle> CreateLayer(const RLDesc& desc);
    ThreadInvocation<utils::ExError> RemoveLayer(const RLHandle& handle);

    ThreadInvocationVoid SortLayers(const Predicate<RLIntern>& pred = RL_SORT_BY_WEIGHT);

    ThreadInvocation<utils::ExResult<RLRegistry>> GetCurrentRegistry();

    ThreadInvocation<utils::ExError> RegisterWork(FramebufferHandle fb);
    ThreadInvocation<utils::ExError> UnRegisterWork();
};

}