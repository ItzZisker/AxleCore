#pragma once

#if defined(_WIN32) && defined(__AX_PLATFORM_WIN32__)
#include "AX_IApplication.hpp"

#ifndef CALLBACK
#if defined(_ARM_)
#define CALLBACK
#else
#define CALLBACK __stdcall
#endif
#endif

using vHINSTANCE = void*;
using vHWND = void*;

using vLRESULT = intptr_t;
using vUINT = uint32_t;
using vWPARAM = uintptr_t;
using vLPARAM = intptr_t;

namespace axle::core {

class ApplicationWin32 : public IApplication {
public:
    ApplicationWin32(const ApplicationSpecification& spec, uint32_t maxSharedEvents = 32);
    ~ApplicationWin32() override;

    void Launch() override;
    void Shutdown() override;

    void PollEvents() override;

    void SetTitle(const std::string& title) override;

    void SetResizable(bool enabled) override;
    void SetCursorMode(CursorMode mode) override;

    void* GetNativeWindowHandle() override {  return m_Hwnd; };

    ApplicationType GetType() const override { return ApplicationType::App_Win32; }
private:
    std::mutex m_HandleMutex;

    bool m_Grabbed = false;
    bool m_Focused = false;
    bool m_IsMouseOnEdge = false;

    vHWND m_Hwnd = nullptr;
    vHINSTANCE m_Instance = nullptr;

    static vLRESULT CALLBACK WndProc(vHWND hwnd, vUINT msg, vWPARAM wParam, vLPARAM lParam);
};

}

#endif