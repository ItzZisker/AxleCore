#pragma once

#include "AX_PCH.hpp"
#include <functional>

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
struct EventWindowResize {
    uint32_t width;
    uint32_t height;
};

struct EventWindowFocus {
    bool focused;
};

struct EventKey {
    unsigned long key;
    bool pressed;
};

struct EventMouseMove {
    float x;
    float y;
};

struct EventMouseButton {
    int button;
    bool pressed;
};

class IApplication {
public:
    virtual ~IApplication() = default;

    // Core lifecycle
    virtual void Launch() = 0;
    virtual void Shutdown() = 0;

    // Run one iteration of event polling
    virtual void PollEvents() = 0;

    // Must be called after PollEvents(), checks if window backbuffer is throttling/blocking so, don't write to it at that moment
    virtual bool IsThrottling() = 0;

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

    // Callbacks
    virtual void SetFocusCallback(std::function<void(const EventWindowFocus&)> func) = 0;
    virtual void SetResizeCallback(std::function<void(const EventWindowResize&)> func) = 0;
    virtual void SetKeyCallback(std::function<void(const EventKey&)> func) = 0;
    virtual void SetMouseMoveCallback(std::function<void(const EventMouseMove&)> func) = 0;
    virtual void SetMouseButtonCallback(std::function<void(const EventMouseButton&)> func) = 0;

    // Backend-specific pointer access (GL/DX/VK surfaces, handles, etc)
    virtual void* GetNativeWindowHandle() const = 0;
public:
    static IApplication* Create(const ApplicationSpecification& spec);
};

}
