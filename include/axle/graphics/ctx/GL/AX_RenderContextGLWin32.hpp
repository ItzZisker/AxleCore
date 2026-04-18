#pragma once

#if defined(__AX_PLATFORM_WIN32__) && defined(__AX_GRAPHICS_GL__)
#include "axle/graphics/ctx/AX_IRenderContext.hpp"

#include <glad/gl.h>

typedef void* vHGLRC;
typedef void* vHDC;
typedef void* vHWND;

namespace axle::gfx {

struct GLCSurfaceInfo {
    uint32_t width{0};
    uint32_t height{0};
    uint32_t colorBits{32};
    uint32_t alphaBits{8};
    uint32_t depthBits{24};
    uint32_t stencilBits{8};
    uint32_t samples{1};
    bool vsync{false};
};

struct GLCHandleWin32 {
    vHGLRC hglrc{0};
    GladGLContext glCtx{};
    GLCSurfaceInfo surfaceInfo{};
};

GLCSurfaceInfo ToGLCSurfaceInfo(SurfaceDesc desc);

class RenderContextGLWin32 : public IRenderContext {
public:
    RenderContextGLWin32();
    virtual ~RenderContextGLWin32();

    utils::ExError Init(SharedPtr<core::IWindow> app, SurfaceDesc desc) override;

    void MakeCurrent();
    void ReleaseCurrent();

    void SwapBuffers() override;
    void SetVSync(bool enabled) override;
    void Shutdown() override;

    SharedPtr<void> GetContextHandle() const override;

    core::WindowType GetAppType() const override { return core::WindowType::Win32; }
    GfxType GetType() const override { return GfxType::GL330; }
private:
    bool m_Initialized{false};

    SharedPtr<GLCHandleWin32> m_Handle{std::make_shared<GLCHandleWin32>()};
    
    vHDC   m_dummyhdc{nullptr};
    vHDC   m_hdc{nullptr};
    vHWND  m_dummyhwnd{nullptr};
    vHWND  m_hwnd{nullptr};

    void* m_Func_wglCreateContextAttribsARB{nullptr};
    void* m_Func_wglChoosePixelFormatARB{nullptr};
    void* m_Func_wglSwapIntervalEXT{nullptr};
};

}
#endif