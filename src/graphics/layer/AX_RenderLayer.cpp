#include "axle/graphics/layer/AX_RenderLayer.hpp"

namespace axle::gfx
{

RenderLayer::RenderLayer(void* userPtr)
    : m_UserPtr(userPtr) {}

RenderLayer::RenderLayer(SharedPtr<void> userPtr)
    : m_shUserPtr(userPtr), m_UserPtr(userPtr.get()) {}

RenderLayer::~RenderLayer() {
    RenderLayer::UnRegisterWork();
}

utils::ExResult<RLHandle> RenderLayer::CreateLayer(const RLDesc& desc) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto& layer = *m_RLPool.Reserve();
    layer.updateFunc = desc.updateFunc;
    layer.drawFunc = desc.drawFunc;
    layer.sortKey = desc.sortKey;
    layer.stage = desc.stage;
    layer.Sign();
    return layer.External();
}

utils::ExError RenderLayer::RemoveLayer(const RLHandle& handle) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (!m_RLPool.IsValid(handle)) return {"Invalid RLHandle"};
    m_RLPool.Delete(handle);
}

void RenderLayer::SortLayers(const Predicate<RLIntern>& pred) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_RLPool.SortOrder(pred);
}

void RenderLayer::Update(float dT) {
    std::lock_guard<std::mutex> lock(m_Mutex);

    if (!m_RLRegistry.active) return;

    auto* thrPtr = m_RLRegistry.gfxThread.get();
    for (auto& orderId : m_RLPool.GetOrder()) {
        auto& raw = m_RLPool.GetRaw(orderId);
        if (raw.updateFunc) {
            raw.updateFunc(thrPtr, dT, m_UserPtr);
        }
    }
}

void RenderLayer::Draw(float dT) {
    std::lock_guard<std::mutex> lock(m_Mutex);

    if (!m_RLRegistry.active) return;

    auto* thrPtr = m_RLRegistry.gfxThread.get();
    for (auto& orderId : m_RLPool.GetOrder()) {
        auto& raw = m_RLPool.GetRaw(orderId);
        if (raw.drawFunc) {
            raw.drawFunc(thrPtr, dT, m_UserPtr);
        }
    }
}

const utils::ExResult<RLRegistry> RenderLayer::GetCurrentRegistry() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (!m_RLRegistry.active) return {"No active registry"};
    return m_RLRegistry;
}

utils::ExError RenderLayer::RegisterWork(FramebufferHandle fb, SharedPtr<core::ThreadContextGfx> thrCtx) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_RLRegistry.active) return {"Already has active registry"};

    m_RLRegistry.active = true;
    m_RLRegistry.framebuffer = fb;
    m_RLRegistry.gfxThread = thrCtx;
    m_RLRegistry.workHandle = thrCtx->CreateWork([this, thrCtx](){
        auto frameTime = thrCtx->GetFrameTime();
        RenderLayer::Update(frameTime);
        RenderLayer::Draw(frameTime);
    });
    return utils::ExError::NoError();
}

utils::ExError RenderLayer::UnRegisterWork() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (!m_RLRegistry.active) return {"No active registry"};

    m_RLRegistry.gfxThread->RemoveWork(m_RLRegistry.workHandle);
    m_RLRegistry.workHandle = {};
    m_RLRegistry.framebuffer = {};
    m_RLRegistry.gfxThread = {nullptr};
    m_RLRegistry.active = false;

    return utils::ExError::NoError();
}

}