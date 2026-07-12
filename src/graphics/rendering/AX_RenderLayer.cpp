#include "axle/graphics/rendering/AX_RenderLayer.hpp"

namespace axle::gfx
{

RenderLayer::RenderLayer(ThreadGfxScope gfxThread, SharedPtr<void> userPtr)
    : ThreadOwned(gfxThread), m_shUserPtr(userPtr), m_UserPtr(userPtr.get()) {}

RenderLayer::RenderLayer(ThreadGfxScope gfxThread, void* userPtr)
    : ThreadOwned(gfxThread), m_UserPtr(userPtr) {}

RenderLayer::~RenderLayer() {
    RenderLayer::UnRegisterWork();
}

void RenderLayer::Update(float dT) {
    if (!m_RLRegistry.active)
        return;

    for (auto& orderId : m_RLPool.GetOrder()) {
        auto& raw = m_RLPool.GetRaw(orderId);
        if (raw.updateFunc) {
            raw.updateFunc(m_Thread, dT, m_UserPtr);
        }
    }
}

void RenderLayer::Draw(float dT) {
    if (!m_RLRegistry.active)
        return;

    for (auto& orderId : m_RLPool.GetOrder()) {
        auto& raw = m_RLPool.GetRaw(orderId);
        if (raw.drawFunc) {
            raw.drawFunc(m_Thread, dT, m_UserPtr);
        }
    }
}

ThreadInvocation<RLHandle> RenderLayer::CreateLayer(const RLDesc& desc) {
    return ThreadInvocation<RLHandle>(m_Thread, [&](){
        auto& layer = *m_RLPool.Reserve();
        layer.updateFunc = desc.updateFunc;
        layer.drawFunc = desc.drawFunc;
        layer.sortKey = desc.sortKey;
        layer.stage = desc.stage;
        layer.Sign();
        return layer.External();
    });
}

ThreadInvocation<utils::ExError> RenderLayer::RemoveLayer(const RLHandle& handle) {
    return ThreadInvocation<utils::ExError>(m_Thread, [&](){
        if (!m_RLPool.IsValid(handle)) return utils::ExError{"Invalid RLHandle"};
        m_RLPool.Delete(handle);
        return utils::ExError::NoError();
    });
}

ThreadInvocationVoid RenderLayer::SortLayers(const Predicate<RLIntern>& pred) {
    return ThreadInvocationVoid(m_Thread, [&](){
        m_RLPool.SortOrder(pred);
        return VoidInvoke{};
    });
}

ThreadInvocation<utils::ExResult<RLRegistry>> RenderLayer::GetCurrentRegistry() {
    return ThreadInvocation<utils::ExResult<RLRegistry>>(m_Thread, [&](){
        if (!m_RLRegistry.active) return utils::ExResult<RLRegistry>(utils::ExError{"No active registry"});
        return m_RLRegistry;
        return utils::ExError::NoError();
    });
}

ThreadInvocation<utils::ExError> RenderLayer::RegisterWork(FramebufferHandle fb) {
    return ThreadInvocation<utils::ExError>(m_Thread, [&](){
        if (m_RLRegistry.active) return utils::ExError{"Already has active registry"};

        m_RLRegistry.active = true;
        m_RLRegistry.framebuffer = fb;
        m_RLRegistry.workHandle = m_Thread->CreateWork([this, gfxThr = m_Thread](){
            auto frameTime = gfxThr->GetFrameTime();
            RenderLayer::Update(frameTime);
            RenderLayer::Draw(frameTime);
        });
        return utils::ExError::NoError();
    });
}

ThreadInvocation<utils::ExError> RenderLayer::UnRegisterWork() {
    return ThreadInvocation<utils::ExError>(m_Thread, [&](){
        if (!m_RLRegistry.active) return utils::ExError{"No active registry"};

        m_Thread->RemoveWork(m_RLRegistry.workHandle);
        m_RLRegistry.workHandle = {};
        m_RLRegistry.framebuffer = {};
        m_RLRegistry.active = false;

        return utils::ExError::NoError();
    });
}

}