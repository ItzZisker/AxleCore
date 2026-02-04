#pragma once

#include "AX_PCH.hpp"

#include <deque>
#include <mutex>
#include <string>

// IMPORTANT!
// We have to implement joystick calliberation/choose or anything support later; 
// evdev/libinput/idk for linux & DirectInput/XInput for windows, for android (yeah we're cooked) & for Mac, I have no idea (we are super-cooked).

namespace axle::core {

enum class WndCursorMode {
    CmNormal,
    CmHidden,
    CmLocked // centered & raw mouse movement
};

struct WindowSpec {
    std::string title = "AX App";
    uint32_t width = 1024;
    uint32_t height = 768;
    bool resizable = true;
};

enum WindowType {
    WndWin32,
    WndX11,
    WndCocoa,
    WndSurfaceFlinger,
    WndUnknown
};

enum WndEventType {
    EvVoid,
    EvKey,
    EvWindowResize,
    EvWindowFocus,
    EvMouseMove,
    EvMouseButton,
    EvQuit
};

union WndEventValue {
    struct { uint64_t key; bool pressed; } key;
    struct { uint32_t width, height; } windowResize;
    struct { bool focused; } windowFocus;
    struct { float x, y; } mouseMove;
    struct { int button; bool pressed; } mouseButton;
};

struct WndEvent {
    WndEventType type{WndEventType::EvVoid};
    WndEventValue value;
};

class SharedState {
public:
    explicit SharedState(WindowSpec spec = {}, uint32_t maxEventsQueue = 32);
    
    bool IsRunning() const;
    bool IsResizable() const;
    bool IsQuitting() const;

    WndCursorMode GetCursorMode() const;

    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    uint32_t GetMaxEventQueueCapacity() const;

    std::string GetTitle() const;

    void PushEvent(const WndEvent& event);

    std::deque<WndEvent> TakeEvents();
protected:
    void SetRunning(bool running);
    void SetResizable(bool resizable);
    void SetTitle(const std::string& title);
    void SetSize(uint32_t width, uint32_t height);
    void SetCursorMode(WndCursorMode mode);

    void RequestQuit();
private:
    friend class IWindow;

    friend class WindowWin32;
    friend class WindowX11;

    WndCursorMode m_CursorMode{WndCursorMode::CmNormal};
    uint32_t m_Width{1024}, m_Height{768};

    uint32_t m_MaxEventQueueCapacity{32};
    std::deque<WndEvent> m_SharedEventsQ;

    bool m_IsRunning{false};
    bool m_IsResizable{false};
    bool m_ShouldQuit{false};

    std::string m_Title{"Pylo App"};

    mutable std::mutex m_Mutex;
};

class IWindow {
public:
    IWindow(const WindowSpec& spec, uint32_t maxSharedEvents) 
        : m_State(spec, maxSharedEvents) {}

    IWindow(const IWindow&) = delete;
    IWindow& operator=(const IWindow&) = delete;

    IWindow(IWindow&&) = delete;
    IWindow& operator=(const IWindow&&) = delete;
    
    virtual ~IWindow() = default;

    virtual void Launch() = 0;
    virtual void Shutdown() = 0;

    virtual void PollEvents() = 0;

    virtual void SetTitle(const std::string& title) = 0;
    virtual void SetResizable(bool enabled) = 0;
    virtual void SetCursorMode(WndCursorMode mode) = 0;

    virtual void RequestQuit() { m_State.RequestQuit(); }
    virtual SharedState& GetSharedState() { return m_State; }

    virtual WindowType GetType() const = 0;

    // Backend-specific pointer access (GL/DX/VK surfaces, handles, etc)
    virtual void* GetNativeWindowHandle() = 0;
public:
    static IWindow* Create(const WindowSpec& spec, uint32_t maxSharedEvents);
    
    static inline WindowType ChooseType() {
#if defined(__AX_PLATFORM_WIN32__)
        return WindowType::WndWin32;
#elif defined(__AX_PLATFORM_X11__)
        return WindowType::WndX11
#else
        throw std::runtime_error("AX Exception: IWindow::ChooseType() Failed -> Platform Unsupported!");
#endif
    }
protected:
    friend class IRenderContext;

    friend class RenderContextGLWin32;
    friend class RenderContextGLX11;
    friend class RenderContextD3D11Win32;

    std::mutex m_HandleMutex;

    mutable SharedState m_State;
};

}
