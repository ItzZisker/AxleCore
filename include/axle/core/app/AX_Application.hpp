#pragma once

#include "axle/core/AX_GameLoop.hpp"
#include "axle/utils/AX_Types.hpp"

#include <atomic>

namespace axle::core
{

struct ApplicationSpec {
    core::WindowSpec wndspec{};
    ChMillis fixedTickRate = ChMillis(50);
};

class Application {
private:
    SharedPtr<core::ThreadContextWnd> m_WndThread{std::make_shared<core::ThreadContextWnd>()};
    SharedPtr<core::ThreadContextGfx> m_GfxThread{std::make_shared<core::ThreadContextGfx>()};

    core::SharedState m_State{};
    std::atomic_bool m_FirstInit{true};
public:
    int InitCurrent(ApplicationSpec spec, TickJob updateFunc);

    void Shutdown();

    SharedPtr<core::ThreadContextWnd> GetWindowThread() const { return m_WndThread; }
    SharedPtr<core::ThreadContextGfx> GetGraphicsThread() const { return m_GfxThread; };

    bool IsRunning() const { return m_State.IsRunning() && !m_State.IsQuitting(); }
    const core::SharedState& GetState() { return m_State; };
};

}