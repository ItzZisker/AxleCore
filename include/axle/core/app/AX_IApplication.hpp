#pragma once

#include "AX_PCH.hpp"

#include <deque>
#include <mutex>
#include <string>

// IMPORTANT!
// We have to implement joystick calliberation/choose or anything support later; 
// evdev/libinput/idk for linux & DirectInput/XInput for windows, for android (yeah we're cooked) & for Mac, I have no idea (we are super-cooked).

namespace axle::core {

enum class CursorMode {
    Normal,
    Hidden,
    Locked // centered & raw mouse movement
};

struct ApplicationSpecification {
    std::string title = "AX App";
    uint32_t width = 1024;
    uint32_t height = 768;
    bool resizable = true;
};

enum EventType {
    Void,
    Key,
    WindowResize,
    WindowFocus,
    MouseMove,
    MouseButton,
    Quit
};

union EventValue {
    struct { uint64_t key; bool pressed; } key;
    struct { uint32_t width, height; } windowResize;
    struct { bool focused; } windowFocus;
    struct { float x, y; } mouseMove;
    struct { int button; bool pressed; } mouseButton;
};

struct Event {
    EventType type = EventType::Void;
    EventValue value;
};

class SharedState {
public:
    explicit SharedState(ApplicationSpecification spec = {}, uint32_t maxEventsQueue = 32);
    
    bool IsRunning() const;
    bool IsResizable() const;
    bool IsQuitting() const;

    CursorMode GetCursorMode() const;

    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    uint32_t GetMaxEventQueueCapacity() const;

    std::string GetTitle() const;

    void PushEvent(const Event& event);

    std::deque<Event> TakeEvents();
protected:
    void SetRunning(bool running);
    void SetResizable(bool resizable);
    void SetTitle(const std::string& title);
    void SetSize(uint32_t width, uint32_t height);
    void SetCursorMode(CursorMode mode);

    void RequestQuit();
private:
    friend class IApplication;

    friend class ApplicationWin32;
    friend class ApplicationX11;

    CursorMode m_CursorMode{CursorMode::Normal};
    uint32_t m_Width{1024}, m_Height{768};

    uint32_t m_MaxEventQueueCapacity{32};
    std::deque<Event> m_SharedEventsQ;

    bool m_IsRunning{false};
    bool m_IsResizable{false};
    bool m_ShouldQuit{false};

    std::string m_Title{"Pylo App"};

    mutable std::mutex m_Mutex;
};

class IApplication {
public:
    IApplication(const ApplicationSpecification& spec, uint32_t maxSharedEvents) 
        : m_State(spec, maxSharedEvents) {}

    IApplication(const IApplication&) = delete;
    IApplication& operator=(const IApplication&) = delete;

    IApplication(IApplication&&) = delete;
    IApplication& operator=(const IApplication&&) = delete;
    
    virtual ~IApplication() = default;

    virtual void Launch() = 0;
    virtual void Shutdown() = 0;

    virtual void PollEvents() = 0;

    virtual void SetTitle(const std::string& title) = 0;
    virtual void SetResizable(bool enabled) = 0;
    virtual void SetCursorMode(CursorMode mode) = 0;

    virtual void RequestQuit() { m_State.RequestQuit(); }
    virtual SharedState& GetSharedState() { return m_State; }

    // Backend-specific pointer access (GL/DX/VK surfaces, handles, etc)
    virtual void* GetNativeWindowHandle() = 0;
public:
    static IApplication* Create(const ApplicationSpecification& spec, uint32_t maxSharedEvents);
protected:
    mutable SharedState m_State;
};

}
