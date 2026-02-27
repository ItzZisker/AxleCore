#include "axle/core/app/AX_Application.hpp"
#include "axle/core/ctx/AX_IRenderContext.hpp"
#include "axle/core/window/AX_IWindow.hpp"

#include "axle/utils/AX_Expected.hpp"
#include "axle/utils/AX_Types.hpp"

#ifdef __AX_GRAPHICS_GL__
#include "axle/graphics/cmd/GL/AX_GLGraphicsBackend.hpp"
#endif

#ifdef __AX_PLATFORM_WIN32__
#include "axle/core/window/AX_WindowWin32.hpp"

#ifdef __AX_GRAPHICS_GL__
#include "axle/core/ctx/GL/AX_RenderContextGLWin32.hpp"
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
int Application::InitCurrent(
    ApplicationSpec spec,
    std::function<void(Application&, void*)> initFunc,
    std::function<void(float, Application&, void*)> updateFunc,
    void* miscData
) {
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
    if (!wndRes) return -1;

    m_WndThread->AwaitStart();

    auto wndCtx = m_WndThread->GetContext();

    bool gfxRes = m_GfxThread->StartGfx([wndCtx, spec = std::move(spec)]() -> ExResult<SharedPtr<gfx::IGraphicsBackend>> {
        IRenderContext* ctxPtr{nullptr};
        int32_t combinedTypes = IRenderContext::CombinedTypes();
        auto& sortedTypesByOS = IRenderContext::SortedTypesByPlatform();
        bool enforced = false;
        if (spec.enforceGfxType) {
            auto enforcedType = spec.enforcedGfxType;
            if (!(combinedTypes & (int)enforcedType)) {
                return AXError("AX Error: Specified enforced gfx type is not supported!");
            }
            enforced = spec.enforceGfxType;
            combinedTypes = (int)enforcedType;
        }
        if (combinedTypes & (int)GfxType::VK) {
#ifdef __AX_GRAPHICS_VK
            //ctxPtr = new VKRenderContext();
            //ctxPtr.TryInit();
#else
            goto _bkPreDX11;
#endif
        }
        _bkPreDX11:
        if (combinedTypes & (int)GfxType::DX11) {
#ifdef __AX_GRAPHICS_DX11
            // TODO
            //ctxPtr = new DX11RenderContext();
            //ctxPtr.TryInit();
#else
            goto _bkPreGL330;
#endif
        }
        _bkPreGL330:
        if (combinedTypes & (int)GfxType::GL330) {
#ifdef __AX_GRAPHICS_GL__
#if defined(__AX_PLATFORM_WIN32__)
            ctxPtr = new RenderContextGLWin32();
#elif defined(__AX_PLATFORM_X11__)
            ctxPtr = new RenderContextGLX11();
#else
            return AXError("AX Error: IWindow::ChooseType() Failed -> Platform Unsupported!");
#endif
#endif
        }
        if (!ctxPtr->Init(wndCtx)) {
            return AXError("AX Error: Couldn't Create enforced-GL Context");
        }
        switch (ctxPtr->GetType()) {
            case GfxType::GL330: return {std::make_shared<gfx::GLGraphicsBackend>(ctxPtr)};
            default: throw std::runtime_error("Bruh");
        }
    });
    if (!gfxRes) return -2;

    m_GfxThread->AwaitStart();
    initFunc(*this, miscData);

    auto& state = wndCtx->GetSharedState();

    using namespace std::chrono;

    auto tickRate = spec.fixedTickRate;
    auto lastTime = steady_clock::now();
    while (!state.IsQuitting()) {
        auto now = steady_clock::now();
        auto delta = duration_cast<milliseconds>(now - lastTime);
        lastTime = now;
        updateFunc(delta.count() / 1000.0f, *this, miscData);
        if (tickRate.count() > 0) {
            std::this_thread::sleep_for(ChMillis(std::max(tickRate.count() - delta.count(), (int64_t)0)));
        }
    }
    if (m_GfxThread->IsRunning()) {
        m_GfxThread->Stop(true);
    }
    if (m_WndThread->IsRunning()) {
        m_WndThread->Stop(true);
    }
    return 0;
}

void Application::RequestQuit() {
    if (m_WndThread->IsRunning()) {
        m_WndThread->GetContext()->RequestQuit();
    }
}

}