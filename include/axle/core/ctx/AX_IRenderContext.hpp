#pragma once

#include "axle/core/window/AX_IWindow.hpp"
#include "axle/utils/AX_Types.hpp"

#include <vector>

namespace axle::core {

enum class GfxType {
    GL330 = (1 << 1),
    DX11 = (1 << 2),
    VK = (1 << 3)
};

class IRenderContext {
public:
    virtual ~IRenderContext() = default;

    virtual bool Init(SharedPtr<IWindow> window) = 0;
    virtual void SwapBuffers() = 0;
    virtual void SetVSync(bool enabled) = 0;
    virtual void Shutdown() = 0;

    virtual SharedPtr<void> GetContextHandle() const = 0;

    virtual WindowType GetAppType() const = 0;
    virtual GfxType GetType() const = 0;

    static inline int32_t CombinedTypes() {
        int32_t supported{0};
#ifdef __AX_GRAPHICS_GL__
        supported |= (int)GfxType::GL330;
#endif
#ifdef __AX_GRAPHICS_DX11__
        supported |= (int)GfxType::DX11;
#endif
#ifdef __AX_GRAPHICS_VK__
        supported |= (int)GfxType::VK;
#endif
        return supported;
    }

    static inline const std::vector<GfxType>& SortedTypesByPlatform() {
#if defined(__AX_PLATFORM_WIN32__)
        static std::vector<GfxType> types{GfxType::VK, GfxType::DX11, GfxType::GL330};
        return types;
#elif defined(__AX_PLATFORM_X11__)
        static std::vector<GfxType> types{GfxType::VK, GfxType::GL330};
        return types;
#endif
    }
};

}