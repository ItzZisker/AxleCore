#pragma once

#include "axle/core/app/AX_IApplication.hpp"
namespace axle::core {

class IRenderContext {
public:
    virtual ~IRenderContext() = default;

    // initialize context for the given native window handle (platform-specific)
    // nativeWindow is the same pointer returned by Application::GetNativeWindowHandle()
    virtual bool Init(IApplication* app) = 0;

    // Make this context current on the calling thread (if applicable)
    virtual void MakeCurrent() = 0;

    // Swap front/back buffers (present)
    virtual void SwapBuffers() = 0;

    // Enable/disable Vertical-Sync (blocking swap)
    virtual void SetVSync(bool enabled) = 0;

    // Destroy/cleanup
    virtual void Shutdown() = 0;

    // Some implementations may expose raw handles (GLXContext, HGLRC, VkSurface, etc.)
    virtual void* GetContextHandle() const = 0;
};

}