#include "axle/core/app/AX_Application.hpp"
#include "axle/core/ctx/AX_IRenderContext.hpp"
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
    m_WndThread->StartApp([wndspec = std::move(spec.wndspec)]() -> utils::ExResult<SharedPtr<IWindow>> {
        IWindow* wndPtr{nullptr};
#if defined(__AX_PLATFORM_WIN32__)
        wndPtr = new WindowWin32(wndspec);
#elif defined(__AX_PLATFORM_X11__)
        wndPtr = new WindowX11(wndspec);
#else
        return utils::AXError("AX Error: IWindow::ChooseType() Failed -> Platform Unsupported!");
#endif
        wndPtr->Launch();
        return SharedPtr<IWindow>(wndPtr);
    });
    m_WndThread->AwaitStart();

    auto wndCtx = m_WndThread->GetContext();

    bool gfxRes = m_GfxThread->StartGfx([wndCtx, spec = std::move(spec)]() -> utils::ExResult<SharedPtr<IRenderContext>> {
        IRenderContext* ctxPtr{nullptr};
        int32_t combinedTypes = IRenderContext::CombinedTypes();
        if (spec.enforceGfxType) {
            auto enforcedType = spec.gfxType;
            if (!(combinedTypes & enforcedType)) {
                return utils::AXError("AX Error: Specified enforced gfx type is not supported!");
            }
            combinedTypes = enforcedType;
        }
        if (combinedTypes & GfxVK) {
            //ctxPtr = new VKRenderContext();
            //ctxPtr.TryInit();
        }
        if (combinedTypes & GfxDX11) {
            //ctxPtr = new DX11RenderContext();
            //ctxPtr.TryInit();
        }
        if (combinedTypes & GfxGL330) {
            // based on host's OS/platform
            //ctxPtr = new RenderContextGLWin32();
            //ctxPtr.TryInit();
        }
        ctxPtr->Init(wndCtx.get());
        // LoadGLFunctions in case its GL Context
        // if (!((core::GLRenderContextWin32*) ctx)->LoadGLFunctions()) {
            // std::cerr << "Couldn't Load GLAD Functions\n";
            // std::exit(1);
        // }
        // ctx->SetVSync(false); // Remove this.
        return SharedPtr<IRenderContext>(ctxPtr);
    });
    m_GfxThread->AwaitStart();

    auto& state = wndCtx->GetSharedState();

    using clock = std::chrono::steady_clock;

    auto lastTime = clock::now();
    while (!state.IsQuitting()) {
        auto now = clock::now();
        std::chrono::duration<double> delta = now - lastTime;
        lastTime = now;
        updateFunc(delta.count());
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