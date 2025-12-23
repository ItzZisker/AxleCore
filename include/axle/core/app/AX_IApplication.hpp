#pragma once

#include "AX_PCH.hpp"

#include <deque>
#include <mutex>
#include <thread>

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

// Base event structure (will extend it later or plug into ECS/event bus)
enum EventType {
    Key,
    WindowResize,
    WindowFocus,
    MouseMove,
    MouseButton
};

union EventValue {
    struct { unsigned long key; bool pressed; } key;
    struct { uint32_t width, height; } windowResize;
    struct { bool focused; } windowFocus;
    struct { float x, y; } mouseMove;
    struct { int button; bool pressed; } mouseButton;
};

struct Event {
    EventType type;
    EventValue value;
};

class IApplication {
public:
    virtual ~IApplication() = default;

    virtual std::thread::id GetThreadId() const = 0;

    // Core lifecycle
    virtual void Launch() = 0;
    virtual void Shutdown() = 0;

    // Run one iteration of event polling
    virtual std::deque<Event> PollEvents() = 0;

    // Window properties
    virtual void SetTitle(const std::string& title) = 0;
    virtual const std::string& GetTitle() const = 0;

    virtual uint32_t GetWidth() const = 0;
    virtual uint32_t GetHeight() const = 0;

    virtual void SetResizable(bool enabled) = 0;
    virtual bool IsResizable() const = 0;

    virtual void SetCursorMode(CursorMode mode) = 0;
    virtual CursorMode GetCursorMode() const = 0;

    virtual void RequestQuit() = 0;
    virtual bool ShouldQuit() const = 0;

    // Backend-specific pointer access (GL/DX/VK surfaces, handles, etc)
    virtual void* GetNativeWindowHandle() const = 0;
public:
    static IApplication* Create(const ApplicationSpecification& spec);
};

}
