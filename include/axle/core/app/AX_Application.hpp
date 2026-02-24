#pragma once

#include "axle/core/concurrency/AX_ThreadCycler.hpp"
#include "axle/core/ctx/AX_IRenderContext.hpp"
#include "axle/core/window/AX_IWindow.hpp"
#include "axle/utils/AX_Types.hpp"

#include <atomic>

namespace axle::core
{

struct ApplicationSpec {
    WindowSpec wndspec{};
    ChMillis fixedTickRate = ChMillis(50);
    bool enforceGfxType{false};
    GfxType enforcedGfxType{GfxType::VK};
};

class Application {
private:
    SharedPtr<ThreadContextWnd> m_WndThread{std::make_shared<ThreadContextWnd>()};
    SharedPtr<ThreadContextGfx> m_GfxThread{std::make_shared<ThreadContextGfx>()};

    core::SharedState m_State{};
    std::atomic_bool m_FirstInit{true};
public:
    int InitCurrent(ApplicationSpec spec, std::function<void(float, Application&, void*)> updateFunc, void* miscData);
    void RequestQuit();

    SharedPtr<ThreadContextWnd> GetWindowThread() const { return m_WndThread; }
    SharedPtr<ThreadContextGfx> GetGraphicsThread() const { return m_GfxThread; };

    bool IsRunning() const { return m_State.IsRunning() && !m_State.IsQuitting(); }
    const SharedState& GetState() { return m_State; };
};

}