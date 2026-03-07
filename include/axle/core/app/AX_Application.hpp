#pragma once

#include "axle/core/window/AX_IWindow.hpp"
#include "axle/core/ctx/AX_IRenderContext.hpp"
#include "axle/core/concurrency/AX_ThreadCycler.hpp"

#include "axle/graphics/AX_Graphics.hpp"

#include "axle/utils/AX_Types.hpp"

#include <atomic>

namespace axle::core
{

struct ApplicationSpec {
    WindowSpec wndspec{};
    ChMillis fixedWndSleep = ChMillis(0);
    ChMillis fixedTickRate = ChMillis(50);
    bool enforceGfxType{false};
    GfxType enforcedGfxType{GfxType::VK};
};

class Application {
private:
    SharedPtr<ThreadContextWnd> m_WndThread{std::make_shared<ThreadContextWnd>()};
    SharedPtr<ThreadContextGfx> m_GfxThread{std::make_shared<ThreadContextGfx>()};

    SharedPtr<gfx::Graphics> m_Graphics{nullptr};

    std::atomic_bool m_FirstInit{true};
public:
    utils::ExError InitCurrent(
        ApplicationSpec spec,
        std::function<void(Application&, void*)> initFunc,
        std::function<void(float, Application&, void*)> updateFunc,
        void* miscData
    );
    utils::ExError RequestQuit();

    SharedPtr<ThreadContextWnd> GetWindowThread() const { return m_WndThread; }
    SharedPtr<ThreadContextGfx> GetGraphicsThread() const { return m_GfxThread; };

    SharedPtr<gfx::Graphics> GetGraphics() const { return m_Graphics; }

    DiscreteState& GetState() { return m_WndThread->GetContext()->GetDiscreteState(); };

    bool IsRunning() {
        auto& state = GetState();
        return state.IsRunning() && !state.IsQuitting();
    }
};

}