#pragma once

#include "axle/core/window/AX_IWindow.hpp"

#include "axle/utils/AX_Expected.hpp"
#include "axle/utils/AX_Types.hpp"

#include <vector>

namespace axle::gfx {

enum class GfxType {
    GL330 = (1 << 0),
    DX11  = (1 << 1),
    VK    = (1 << 2)
};

enum class SurfaceColors {
    R8G8B8A8_Unorm,
    R8G8B8A8_sRGB,
    R10G10B10A2_Unorm_Pack32,
    R16G16B16A16_sFloat
};

enum class SurfaceDepthStencil {
    D16_Unorm,
    D24_Unorm_S8_UInt,
    D32_sFloat
};

enum class SurfaceSamples {
    Disabled,
    MSAA_2,
    MSAA_4,
    MSAA_8
};

inline uint32_t GetColorBits(SurfaceColors color) {
    switch (color) {
        case SurfaceColors::R8G8B8A8_Unorm:             return 32U;
        case SurfaceColors::R8G8B8A8_sRGB:              return 32U;
        case SurfaceColors::R10G10B10A2_Unorm_Pack32:   return 32U;
        case SurfaceColors::R16G16B16A16_sFloat:        return 64U;
    }
}

inline uint32_t GetAlphaBits(SurfaceColors color) {
    switch (color) {
        case SurfaceColors::R8G8B8A8_Unorm:             return 8U;
        case SurfaceColors::R8G8B8A8_sRGB:              return 8U;
        case SurfaceColors::R10G10B10A2_Unorm_Pack32:   return 2U;
        case SurfaceColors::R16G16B16A16_sFloat:        return 16U;
    }
}

inline uint32_t GetDepthBits(SurfaceDepthStencil ds) {
    switch (ds) {
        case SurfaceDepthStencil::D16_Unorm:            return 16U;
        case SurfaceDepthStencil::D24_Unorm_S8_UInt:    return 24U;
        case SurfaceDepthStencil::D32_sFloat:           return 32U;
    }
}

inline uint32_t GetStencilBits(SurfaceDepthStencil ds) {
    switch (ds) {
        case SurfaceDepthStencil::D32_sFloat:
        case SurfaceDepthStencil::D16_Unorm:            return 0U;
        case SurfaceDepthStencil::D24_Unorm_S8_UInt:    return 8U;
    }
}

inline uint32_t GetSampleCount(SurfaceSamples samples) {
    switch (samples) {
        case SurfaceSamples::Disabled:  return 1U;
        case SurfaceSamples::MSAA_2:    return 2U;
        case SurfaceSamples::MSAA_4:    return 4U;
        case SurfaceSamples::MSAA_8:    return 8U;
    }
}

struct SurfaceDesc {
    uint32_t width{0};
    uint32_t height{0};
    SurfaceColors colors{SurfaceColors::R8G8B8A8_Unorm};
    SurfaceDepthStencil depthStencil{SurfaceDepthStencil::D24_Unorm_S8_UInt};
    SurfaceSamples samples{SurfaceSamples::Disabled};
    bool vsync{false}; 
};

class IRenderContext {
public:
    virtual ~IRenderContext() = default;

    virtual utils::ExError Init(SharedPtr<core::IWindow> window, SurfaceDesc desc) = 0;

    virtual void SwapBuffers() = 0;
    virtual void SetVSync(bool enabled) = 0;
    virtual void Shutdown() = 0;

    virtual SharedPtr<void> GetContextHandle() const = 0;

    virtual core::WindowType GetAppType() const = 0;
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