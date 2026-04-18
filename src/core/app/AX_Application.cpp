#include "axle/core/app/AX_Application.hpp"
#include "axle/core/window/AX_IWindow.hpp"

#include "axle/graphics/ctx/AX_IRenderContext.hpp"

#include "axle/utils/AX_Expected.hpp"
#include "axle/utils/AX_Types.hpp"

#ifdef __AX_GRAPHICS_GL__
#include "axle/graphics/cmd/GL/AX_GLGraphicsBackend.hpp"
#endif

#ifdef __AX_PLATFORM_WIN32__
#include "axle/core/window/AX_WindowWin32.hpp"

#ifdef __AX_GRAPHICS_GL__
#include "axle/graphics/ctx/GL/AX_RenderContextGLWin32.hpp"
#endif
#endif

#ifdef __AX_PLATFORM_X11
#include "axle/core/window/AX_WindowX11.hpp"
#endif

#include <algorithm>
#include <memory>

namespace axle::core
{

// TODO: Support Android Cycles (NOTE: Main thread == UI) ASAP!!!
utils::ExError Application::InitCurrent(
    ApplicationSpec spec,
    std::function<void(Application&, void*)> initFunc,
    std::function<void(float, Application&, void*)> updateFunc,
    void* miscData
) {
    using namespace axle::utils;

    // m_WndThread->SetCycleTimeCap(spec.fixedWndRate);
    utils::ExError wndRes = m_WndThread->StartApp([wndspec = std::move(spec.wndspec)]() -> ExResult<SharedPtr<IWindow>> {
        IWindow* wndPtr{nullptr};
#if defined(__AX_PLATFORM_WIN32__)
        wndPtr = new WindowWin32(wndspec);
#elif defined(__AX_PLATFORM_X11__)
        wndPtr = new WindowX11(wndspec);
#else
        return ExError("AX Error: IWindow::ChooseType() Failed -> Platform Unsupported!");
#endif
        AX_PROPAGATE_ERROR(wndPtr->Launch());
        return SharedPtr<IWindow>(wndPtr);
    });
    if (wndRes.IsValid()) {
        return wndRes;
    }

    m_WndThread->AwaitStart();

    auto wndCtx = m_WndThread->GetContext();

    utils::ExError gfxRes = m_GfxThread->StartGfx([wndCtx, spec = std::move(spec)]() -> ExResult<SharedPtr<gfx::IGraphicsBackend>> {
        gfx::IRenderContext* ctxPtr{nullptr};
        int32_t combinedTypes = gfx::IRenderContext::CombinedTypes();
        auto& sortedTypesByOS = gfx::IRenderContext::SortedTypesByPlatform();
        bool enforced = false;
        if (spec.enforceGfxType) {
            auto enforcedType = spec.enforcedGfxType;
            if (!(combinedTypes & (int)enforcedType)) {
                return ExError("AX Error: Specified enforced gfx type is not supported!");
            }
            enforced = spec.enforceGfxType;
            combinedTypes = (int)enforcedType;
        }
        if (combinedTypes & (int)gfx::GfxType::VK) {
#ifdef __AX_GRAPHICS_VK
            //ctxPtr = new gfx::VKRenderContext();
            //ctxPtr.TryInit();
#else
            goto _bkPreDX11;
#endif
        }
        _bkPreDX11:
        if (combinedTypes & (int)gfx::GfxType::DX11) {
#ifdef __AX_GRAPHICS_DX11
            // TODO
            //ctxPtr = new gfx::DX11RenderContext();
            //ctxPtr.TryInit();
#else
            goto _bkPreGL330;
#endif
        }
        _bkPreGL330:
        if (combinedTypes & (int)gfx::GfxType::GL330) {
#ifdef __AX_GRAPHICS_GL__
#if defined(__AX_PLATFORM_WIN32__)
            ctxPtr = new gfx::RenderContextGLWin32();
#elif defined(__AX_PLATFORM_X11__)
            ctxPtr = new gfx::RenderContextGLX11();
#else
            return ExError("AX Error: IWindow::ChooseType() Failed -> Platform Unsupported!");
#endif
#endif
        }
        AX_PROPAGATE_ERROR(ctxPtr->Init(wndCtx, spec.surfaceDesc));
        switch (ctxPtr->GetType()) {
            case gfx::GfxType::GL330: return {std::make_shared<gfx::GLGraphicsBackend>(ctxPtr)};
            default: ExError("Unsupported GfxType");
        }
    });
    if (gfxRes.IsValid()) {
        m_WndThread->Stop(true);
        return gfxRes;
    }

    m_GfxThread->AwaitStart();
    m_Graphics = std::make_shared<gfx::Graphics>(m_GfxThread);

    initFunc(*this, miscData);

    auto& state = wndCtx->GetDiscreteState();

    using namespace std::chrono;

    auto tickRate = spec.fixedTickRate;
    auto lastTime = steady_clock::now();
    while (!state.IsQuitting()) {
        auto now = steady_clock::now();
        auto delta = duration_cast<ChMillis>(now - lastTime);
        lastTime = now;
        updateFunc(float(delta.count() / 1e9), *this, miscData);
        if (tickRate.count() > 0) {
            std::this_thread::sleep_for(ChMillis(std::max(tickRate.count() - delta.count(), (int64_t)0)));
        }
    }
    
    if (m_GfxThread->IsRunning()) {
        m_GfxThread->Stop(true);
    }
    m_Graphics.reset();

    if (m_WndThread->IsRunning()) {
        m_WndThread->Stop(true);
    }
    
    return utils::ExError::NoError();
}

utils::ExError Application::RequestQuit() {
    if (m_WndThread->IsRunning()) {
        m_WndThread->GetContext()->RequestQuit();
    }
    return utils::ExError::NoError();
}

}