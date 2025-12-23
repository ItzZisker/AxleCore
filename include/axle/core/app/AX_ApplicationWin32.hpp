#pragma once

#include <mutex>
#if defined(_WIN32) && defined(__AX_PLATFORM_WIN32__)
#include "AX_IApplication.hpp"
#include "axle/tick/AX_Executor.hpp"

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
    ApplicationWin32(const ApplicationSpecification& spec);
    ~ApplicationWin32() override;

    tick::Executor GetExecutor(); // TODO: <- this

    void Launch() override;
    void Shutdown() override;

    std::deque<Event> PollEvents() override;
    void PollEventsInternal();

    void SetTitle(const std::string& title) override;
    const std::string& GetTitle() const override;

    uint32_t GetWidth() const override;
    uint32_t GetHeight() const override;

    void SetResizable(bool enabled) override;
    bool IsResizable() const override;

    void SetCursorMode(CursorMode mode) override;
    CursorMode GetCursorMode() const override;

    void RequestQuit() override;
    bool ShouldQuit() const override;

    void* GetNativeWindowHandle() const override;
private:
    static vLRESULT CALLBACK WndProc(vHWND hwnd, vUINT msg, vWPARAM wParam, vLPARAM lParam);
private:
    std::string m_Title;
    uint32_t m_Width, m_Height;
    bool m_Resizable = true;
    bool m_ShouldQuit = false;

    bool m_Grabbed = false;
    bool m_Focused = false;
    bool m_IsMouseOnEdge = false;

    int32_t m_DequeMaxEvents = 16;

    CursorMode m_CursorMode = CursorMode::Normal;

    vHWND m_Hwnd = nullptr;
    vHINSTANCE m_Instance = nullptr;

    tick::Executor m_Executor;
    std::mutex m_Mutex;
};

}

#endif