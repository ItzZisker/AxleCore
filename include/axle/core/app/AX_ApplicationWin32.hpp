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
    ApplicationWin32(const ApplicationSpecification& spec);
    ~ApplicationWin32() override;

    void Launch() override;
    void Shutdown() override;
    void PollEvents() override;

    void SetTitle(const std::string& title) override;
    const std::string& GetTitle() const override { return m_Title; }

    uint32_t GetWidth() const override { return m_Width; }
    uint32_t GetHeight() const override { return m_Height; }

    void SetResizable(bool enabled) override;
    bool IsResizable() const override { return m_Resizable; }

    void SetCursorMode(CursorMode mode) override;
    CursorMode GetCursorMode() const override { return m_CursorMode; }

    void RequestQuit() override { m_ShouldQuit = true; }
    bool ShouldQuit() const override { return m_ShouldQuit; }

    void SetResizeCallback(std::function<void(const EventWindowResize&)> func) override { m_ResizeCallback = std::move(func); }
    void SetKeyCallback(std::function<void(const EventKey&)> func) override { m_KeyCallback = std::move(func); }
    void SetMouseMoveCallback(std::function<void(const EventMouseMove&)> func) override { m_MouseMoveCallback = std::move(func); }
    void SetMouseButtonCallback(std::function<void(const EventMouseButton&)> func) override { m_MouseButtonCallback = std::move(func); }

    void* GetNativeWindowHandle() const override { return (void*)m_Hwnd; }
private:
    static vLRESULT CALLBACK WndProc(vHWND hwnd, vUINT msg, vWPARAM wParam, vLPARAM lParam);
private:
    std::string m_Title;
    uint32_t m_Width, m_Height;
    bool m_Resizable = true;
    bool m_ShouldQuit = false;

    CursorMode m_CursorMode = CursorMode::Normal;

    vHWND m_Hwnd = nullptr;
    vHINSTANCE m_Instance = nullptr;

    std::function<void(const EventKey&)> m_KeyCallback;
    std::function<void(const EventWindowResize&)> m_ResizeCallback;
    std::function<void(const EventMouseMove&)> m_MouseMoveCallback;
    std::function<void(const EventMouseButton&)> m_MouseButtonCallback;
};

}

#endif