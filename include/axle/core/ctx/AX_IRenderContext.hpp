#pragma once

#include "axle/core/window/AX_IWindow.hpp"

#ifdef __AX_GRAPHICS_GL__
#include <glad/glad.h>
#endif

namespace axle::core {

enum GfxType {
    GfxGL330,
    GfxDX11,
    GfxVK
};

class IRenderContext {
public:
    virtual ~IRenderContext() = default;

    // initialize context for the given window (WindowQtWdgt, WindowWin32, etc.)
    virtual bool Init(IWindow* window) = 0;

    // Swap front/back buffers (present)
    virtual void SwapBuffers() = 0;

    // Enable/disable Vertical-Sync (blocking swap)
    virtual void SetVSync(bool enabled) = 0;

    // Destroy/cleanup
    virtual void Shutdown() = 0;

    // Some implementations may expose raw handles (GLXContext, HGLRC, VkSurface, etc.)
    virtual void* GetContextHandle() const = 0;

    virtual WindowType GetAppType() const = 0;
    virtual GfxType GetType() const = 0;
#ifdef __AX_GRAPHICS_GL__
    // Load GLAD 3.3 Functions Directly by some wrapper/custom/platform context loader
    inline bool LoadGLFunctionsDirect(void *contextLoader) {
        if (contextLoader) return gladLoadGLLoader((GLADloadproc)contextLoader);
        else return false;
    }
#endif
};

}