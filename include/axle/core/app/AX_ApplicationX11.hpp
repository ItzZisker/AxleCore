#pragma once

#ifdef __AX_PLATFORM_X11__
#include "AX_IApplication.hpp"
#include "AX_PCH.hpp"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

namespace axle::core {

class ApplicationX11 : public IApplication {
public:
    explicit ApplicationX11(const ApplicationSpecification& spec);
    virtual ~ApplicationX11();

    // lifecycle
    void Launch() override;
    void Shutdown() override;
    void PollEvents() override;

    bool IsThrottling() override;

    // window
    void SetTitle(const std::string& title) override;
    const std::string& GetTitle() const override { return m_Spec.title; }

    uint32_t GetWidth() const override { return m_Spec.width; }
    uint32_t GetHeight() const override { return m_Spec.height; }

    void SetResizable(bool enabled) override;
    bool IsResizable() const override { return m_Spec.resizable; }

    void SetCursorMode(CursorMode mode) override;
    CursorMode GetCursorMode() const override { return m_CursorMode; }

    void RequestQuit() override { m_ShouldQuit = true; }
    bool ShouldQuit() const override { return m_ShouldQuit; }

    // callbacks
    void SetFocusCallback(std::function<void(const EventWindowFocus&)> func) override;
    void SetResizeCallback(std::function<void(const EventWindowResize&)> func) override;
    void SetKeyCallback(std::function<void(const EventKey&)> func) override;
    void SetMouseMoveCallback(std::function<void(const EventMouseMove&)> func) override;
    void SetMouseButtonCallback(std::function<void(const EventMouseButton&)> func) override;

    // native handle — returns X11 Window (as void*)
    void* GetNativeWindowHandle() const override { return reinterpret_cast<void*>(m_Window); }

    // Access to Display* if caller needs it
    Display* GetDisplay() const { return m_Display; }

    void TranslateAndDispatchKeyEvent(XEvent const& xev);
    void TranslateAndDispatchMouseEvent(XEvent const& xev);

    void CreateWindow(void *visual_info = nullptr);
    void DestroyWindow();
private:
    ApplicationSpecification m_Spec;

    Display* m_Display = nullptr;
    int m_Screen = 0;
    Window m_Window = 0;
    Atom m_WMDeleteWindow = 0;

    bool m_ShouldQuit = false;
    CursorMode m_CursorMode = CursorMode::Normal;

    Cursor m_NormalCursor = 0;
    Cursor m_HiddenCursor = 0;

    std::function<void(const EventWindowFocus&)> m_FocusCallback;
    std::function<void(const EventWindowResize&)> m_ResizeCallback;
    std::function<void(const EventKey&)> m_KeyCallback;
    std::function<void(const EventMouseMove&)> m_MouseMoveCallback;
    std::function<void(const EventMouseButton&)> m_MouseButtonCallback;
};

}
#endif
