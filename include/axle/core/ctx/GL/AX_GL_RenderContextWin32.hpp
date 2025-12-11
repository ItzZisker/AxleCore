#pragma once

#if defined(__AX_PLATFORM_WIN32__) && defined(__AX_GRAPHICS_GL__)
#include "../AX_IRenderContext.hpp"

typedef void* vHGLRC;
typedef void* vHDC;
typedef void* vHWND;

namespace axle::core {

class GL_RenderContextWin32 : public IRenderContext {
public:
    GL_RenderContextWin32();
    virtual ~GL_RenderContextWin32();

    bool Init(IApplication* app) override;
    void MakeCurrent();
    void SwapBuffers() override;
    void SetVSync(bool enabled) override;
    void Shutdown() override;

    void* GetContextHandle() const override;

    bool LoadGLFunctions();
private:
    vHGLRC m_hglrc = nullptr;
    vHDC   m_hdc = nullptr;
    vHWND  m_hwnd = nullptr;
    bool m_Initialized = false;
};

}
#endif