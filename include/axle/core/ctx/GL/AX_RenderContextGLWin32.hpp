#pragma once

#include "axle/core/ctx/AX_IRenderContext.hpp"
#if defined(__AX_PLATFORM_WIN32__) && defined(__AX_GRAPHICS_GL__)
#include "../AX_IRenderContext.hpp"

#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB       0x00000002
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB                0x00000001
#define WGL_CONTEXT_DEBUG_BIT_ARB                       0x00000001
#define WGL_CONTEXT_ES2_PROFILE_BIT_EXT                 0x00000004
#define WGL_CONTEXT_ES_PROFILE_BIT_EXT                  0x00000004
#define WGL_CONTEXT_FLAGS_ARB                           0x2094
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB          0x00000002
#define WGL_CONTEXT_LAYER_PLANE_ARB                     0x2093
#define WGL_CONTEXT_MAJOR_VERSION_ARB                   0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB                   0x2092
#define WGL_CONTEXT_OPENGL_NO_ERROR_ARB                 0x31B3
#define WGL_CONTEXT_PROFILE_MASK_ARB                    0x9126
#define WGL_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB     0x8256
#define WGL_CONTEXT_ROBUST_ACCESS_BIT_ARB               0x00000004
#define WGL_CONTEXT_RELEASE_BEHAVIOR_ARB                0x2097
#define WGL_CONTEXT_RELEASE_BEHAVIOR_FLUSH_ARB          0x2098
#define WGL_CONTEXT_RELEASE_BEHAVIOR_NONE_ARB           0

#include <glad/gl.h>

typedef void* vHGLRC;
typedef void* vHDC;
typedef void* vHWND;

namespace axle::core {

struct GLCSurfaceInfo {
    uint32_t width{0};
    uint32_t height{0};
    uint32_t colorBits{32};
    uint32_t depthBits{24};
    uint32_t stenctilBits{8};
    bool vsync{false};
};

struct GLCHandleWin32 {
    vHGLRC hglrc{0};
    GladGLContext glCtx{};
    GLCSurfaceInfo surfaceInfo{};
};

class RenderContextGLWin32 : public IRenderContext {
public:
    RenderContextGLWin32();
    virtual ~RenderContextGLWin32();

    bool Init(SharedPtr<IWindow> app) override;

    void MakeCurrent();
    void ReleaseCurrent();

    void SwapBuffers() override;
    void SetVSync(bool enabled) override;
    void Shutdown() override;

    SharedPtr<void> GetContextHandle() const override;

    WindowType GetAppType() const override { return WindowType::Win32; }
    GfxType GetType() const override { return GfxType::GL330; }
private:
    bool m_Initialized{false};
    GLCSurfaceInfo m_SurfaceInfo{};

    SharedPtr<GLCHandleWin32> m_Handle{std::make_shared<GLCHandleWin32>()};
    
    vHDC   m_hdc{nullptr};
    vHWND  m_hwnd{nullptr};

    void* m_Func_wglCreateContextAttribsARB{nullptr};
    void* m_Func_wglSwapIntervalEXT{nullptr};
};

}
#endif