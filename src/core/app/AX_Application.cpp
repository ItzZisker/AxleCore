#include "axle/core/app/AX_Application.hpp"
#include "axle/core/ctx/AX_IRenderContext.hpp"
#include "axle/core/ctx/GL/AX_RenderContextGLWin32.hpp"
#include "axle/core/window/AX_IWindow.hpp"

#include "axle/utils/AX_Expected.hpp"
#include "axle/utils/AX_Types.hpp"

#ifdef __AX_PLATFORM_WIN32__
#include "axle/core/window/AX_WindowWin32.hpp"
#endif
#ifdef __AX_PLATFORM_X11
#include "axle/core/window/AX_WindowX11.hpp"
#endif

namespace axle::core
{

int Application::InitCurrent(ApplicationSpec spec, TickJob updateFunc) {
    using namespace axle::utils;

    bool wndRes = m_WndThread->StartApp([wndspec = std::move(spec.wndspec)]() -> ExResult<SharedPtr<IWindow>> {
        IWindow* wndPtr{nullptr};
#if defined(__AX_PLATFORM_WIN32__)
        wndPtr = new WindowWin32(wndspec);
#elif defined(__AX_PLATFORM_X11__)
        wndPtr = new WindowX11(wndspec);
#else
        return AXError("AX Error: IWindow::ChooseType() Failed -> Platform Unsupported!");
#endif
        wndPtr->Launch();
        return SharedPtr<IWindow>(wndPtr);
    });
    if (!wndRes) return false;

    m_WndThread->AwaitStart();

    auto wndCtx = m_WndThread->GetContext();

    bool gfxRes = m_GfxThread->StartGfx([wndCtx, spec = std::move(spec)]() -> ExResult<SharedPtr<IRenderContext>> {
        IRenderContext* ctxPtr{nullptr};
        int32_t combinedTypes = IRenderContext::CombinedTypes();
        auto& sortedTypesByOS = IRenderContext::SortedTypesByPlatform();
        bool enforced = false;
        if (spec.enforceGfxType) {
            auto enforcedType = spec.enforcedGfxType;
            if (!(combinedTypes & enforcedType)) {
                return AXError("AX Error: Specified enforced gfx type is not supported!");
            }
            enforced = spec.enforceGfxType;
            combinedTypes = enforcedType;
        }
        if (combinedTypes & GfxVK) {
            //ctxPtr = new VKRenderContext();
            //ctxPtr.TryInit();
        }
        _bkPreDX11:
        if (combinedTypes & GfxDX11) {
#ifdef __AX_GRAPHICS_DX11
            // TODO
            //ctxPtr = new DX11RenderContext();
            //ctxPtr.TryInit();
#else
            return AXError("AX Error: IWindow::ChooseType() Failed -> Platform Unsupported!");
#endif
        }
        // LoadGLFunctions in case its GL Context
        // if (!((core::GLRenderContextWin32*) ctx)->LoadGLFunctions()) {
            // std::cerr << "Couldn't Load GLAD Functions\n";
            // std::exit(1);
        // }
        // ctx->SetVSync(false); // Remove this.
        _bkPreGL330:
        if (combinedTypes & GfxGL330) {
#ifdef __AX_GRAPHICS_GL__
#if defined(__AX_PLATFORM_WIN32__)
            ctxPtr = new RenderContextGLWin32();
            if (!ctxPtr->Init(wndCtx.get())) {
                return AXError("AX Error: Couldn't Create enforced-GL Context");
            }
            if (!((RenderContextGLWin32*) ctxPtr)->LoadGLFunctions()) {
                return AXError("AX Error: Couldn't load enforced-GLAD Functions");
            }
#elif defined(__AX_PLATFORM_X11__)
            ctxPtr = new RenderContextGLX11();
            ctxPtr->Init(wndCtx.get());
            if (!((RenderContextGLX11*) ctxPtr)->LoadGLFunctions()) {
                return AXError("AX Error: Couldn't load GLAD Functions");
            }
#else
            return AXError("AX Error: IWindow::ChooseType() Failed -> Platform Unsupported!");
#endif
#endif
        }
        return SharedPtr<IRenderContext>(ctxPtr);
    });
    if (!gfxRes) return false;

    m_GfxThread->AwaitStart();

    auto& state = wndCtx->GetSharedState();

    using clock = std::chrono::steady_clock;

    auto tickRate = spec.fixedTickRate;
    auto lastTime = clock::now();
    while (!state.IsQuitting()) {
        auto now = clock::now();
        std::chrono::duration<double> delta = now - lastTime;
        lastTime = now;
        updateFunc(delta.count());
        if (tickRate.count() > 0) {
            std::this_thread::sleep_for(tickRate);
        }
    }
    if (m_GfxThread->IsRunning()) {
        m_GfxThread->Stop(true);
    }
    if (m_WndThread->IsRunning()) {
        m_WndThread->Stop(true);
    }
    return true;
}

void Application::RequestQuit() {
    if (m_WndThread->IsRunning()) {
        m_WndThread->GetContext()->RequestQuit();
    }
}

}