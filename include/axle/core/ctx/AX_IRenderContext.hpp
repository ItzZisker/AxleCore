#pragma once

#include "axle/core/window/AX_IWindow.hpp"
#include <vector>

namespace axle::core {

enum GfxType {
    GfxGL330 = (1 << 1),
    GfxDX11 = (1 << 2),
    GfxVK = (1 << 3)
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

    static inline int32_t CombinedTypes() {
        int32_t supported{0};
#ifdef __AX_GRAPHICS_GL__
        supported |= GfxGL330;
#endif
#ifdef __AX_GRAPHICS_DX11__
        supported |= GfxDX11;
#endif
#ifdef __AX_GRAPHICS_VK__
        supported |= GfxVK;
#endif
        return supported;
    }

    static inline const std::vector<GfxType>& SortedTypesByPlatform() {
#if defined(__AX_PLATFORM_WIN32__)
        static std::vector<GfxType> types{GfxVK, GfxDX11, GfxGL330};
        return types;
#elif defined(__AX_PLATFORM_X11__)
        static std::vector<GfxType> types{GfxVK, GfxGL330};
        return types;
#endif
    }
};

}