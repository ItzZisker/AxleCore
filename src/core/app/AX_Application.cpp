#include "axle/core/app/AX_Application.hpp"
#include "axle/core/AX_GameLoop.hpp"
#include "axle/core/ctx/AX_IRenderContext.hpp"
#include "axle/core/window/AX_IWindow.hpp"
#include <stdexcept>

#ifdef __AX_PLATFORM_WIN32__
#include "axle/core/window/AX_WindowWin32.hpp"
#endif

#ifdef __AX_PLATFORM_X11
#include "axle/core/window/AX_WindowX11.hpp"
#endif

namespace axle::core
{

int Application::InitCurrent(ApplicationSpec spec, TickJob updateFunc) {
    m_WndThread->StartApp([wndspec = std::move(spec.wndspec)]() -> SharedPtr<IWindow> {
        IWindow* wndPtr{nullptr};
        WindowType wndType = IWindow::ChooseType();
        switch (wndType) {
            case WndWin32:
                wndPtr = new WindowWin32(wndspec);
            break;
            case WndX11:
                //wndPtr = new WindowX11(wndspec);
            break;
            case WndCocoa:
            case WndSurfaceFlinger:
            case WndUnknown:
            break;
        }
        // TODO: wndPtr shouldn't be null!!, Implement them based on platform
        wndPtr->Launch();
        return SharedPtr<IWindow>(wndPtr);
    });
    m_WndThread->AwaitStart();

    auto wndCtx = m_WndThread->GetContext();

    m_GfxThread->StartGfx([wndCtx, spec = std::move(spec)]() -> SharedPtr<IRenderContext> {
        IRenderContext* ctxPtr{nullptr};
        int32_t combinedTypes = IRenderContext::CombinedTypes();
        // check if spec is enforced a singular gfx backend, when verify if its in the combined types then try creating only that.
        if (combinedTypes & GfxVK) {
            //ctxPtr = new VKRenderContext();
            //ctxPtr.TryInit();
            //if failed go to next...
        }
        if (combinedTypes & GfxDX11)
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



    while ()
}

void Application::Shutdown() {

}

}