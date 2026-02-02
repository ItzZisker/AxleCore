#pragma once

#if defined(__AX_PLATFORM_WIN32__) && defined(__AX_GRAPHICS_GL__)
#include "../AX_IRenderContext.hpp"

typedef void* vHGLRC;
typedef void* vHDC;
typedef void* vHWND;

namespace axle::core {

class RenderContextGLWin32 : public IRenderContext {
public:
    RenderContextGLWin32();
    virtual ~RenderContextGLWin32();

    bool Init(IWindow* app) override;
    void MakeCurrent();
    void SwapBuffers() override;
    void SetVSync(bool enabled) override;
    void Shutdown() override;

    void* GetContextHandle() const override;

    bool LoadGLFunctions();

    WindowType GetAppType() const override { return WndWin32; }
    GfxType GetType() const override { return GfxGL330; }
private:
    vHGLRC m_hglrc = nullptr;
    vHDC   m_hdc = nullptr;
    vHWND  m_hwnd = nullptr;
    bool m_Initialized = false;
};

}
#endif