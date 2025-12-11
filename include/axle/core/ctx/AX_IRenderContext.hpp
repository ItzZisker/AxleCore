#pragma once

#include "axle/core/app/AX_IApplication.hpp"

#ifdef __AX_GRAPHICS_GL__
#include <glad/glad.h>
#endif

namespace axle::core {

class IRenderContext {
public:
    virtual ~IRenderContext() = default;

    // initialize context for the given native window handle (platform-specific)
    // nativeWindow is the same pointer returned by Application::GetNativeWindowHandle()
    virtual bool Init(IApplication* app) = 0;

    // Swap front/back buffers (present)
    virtual void SwapBuffers() = 0;

    // Enable/disable Vertical-Sync (blocking swap)
    virtual void SetVSync(bool enabled) = 0;

    // Destroy/cleanup
    virtual void Shutdown() = 0;

    // Some implementations may expose raw handles (GLXContext, HGLRC, VkSurface, etc.)
    virtual void* GetContextHandle() const = 0;

#ifdef __AX_GRAPHICS_GL__
    // Define GLAD 3.3 Functions, must be called post Context Creation.
    // Returns true if success, false if none or atleast one of functions failed to load.
    virtual bool LoadGLFunctions() = 0;

    // Load GLAD 3.3 Functions Directly by some wrapper/custom/platform context loader
    inline bool LoadGLFunctionsDirect(void *contextLoader) {
        if (contextLoader) return gladLoadGLLoader((GLADloadproc)contextLoader);
        else return false;
    }
#endif
};

}