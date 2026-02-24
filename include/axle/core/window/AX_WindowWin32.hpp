#pragma once

#if defined(_WIN32) && defined(__AX_PLATFORM_WIN32__)
#include "AX_IWindow.hpp"

#ifndef AX_CALLBACK
#if defined(_ARM_)
#define AX_CALLBACK
#else
#define AX_CALLBACK __stdcall
#endif
#endif

using vHINSTANCE = void*;
using vHWND      = void*;

using vLRESULT = intptr_t;
using vUINT    = uint32_t;
using vWPARAM  = uintptr_t;
using vLPARAM  = intptr_t;

namespace axle::core {

class WindowWin32 : public IWindow {
public:
    WindowWin32(const WindowSpec& spec, uint32_t maxSharedEvents = 32);
    ~WindowWin32() override;

    void Launch() override;
    void Shutdown() override;
    void PollEvents() override;

    void SetTitle(const std::string& title) override;
    void SetResizable(bool enabled) override;
    void SetCursorMode(WndCursorMode mode) override;

    void* GetNativeWindowHandle() override {  return m_Hwnd; };

    WindowType GetType() const override { return WindowType::Win32; }
private:
    std::mutex m_HandleMutex;

    bool m_Grabbed = false;
    bool m_Focused = false;
    bool m_MouseOnEdge = false;

    vHWND m_Hwnd = nullptr;
    vHINSTANCE m_Instance = nullptr;

    static vLRESULT AX_CALLBACK WndProc(vHWND hwnd, vUINT msg, vWPARAM wParam, vLPARAM lParam);
};

}

#endif