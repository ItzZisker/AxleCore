#pragma once

#include "axle/core/app/AX_IApplication.hpp"
#if defined(__AX_PLATFORM_X11__) && defined(__AX_GRAPHICS_GL__)
#include "../AX_IRenderContext.hpp"

#include <X11/Xlib.h>

struct __GLXcontextRec;
using GLXContext = struct __GLXcontextRec*;

namespace axle::core {

class GL_RenderContextX11 : public IRenderContext {
public:
    GL_RenderContextX11();
    virtual ~GL_RenderContextX11();

    bool Init(IApplication* app) override;
    void MakeCurrent();
    void SwapBuffers() override;
    void SetVSync(bool enabled) override;
    void Shutdown() override;

    void* GetContextHandle() const override;

    // One-time libGL handle (or libOpenGL)
    void* OpenLibGLOnce();

    /*
    * Low-level symbol resolver:
    *   1) Try glXGetProcAddressARB
    *   2) Fallback to dlsym on libGL or libOpenGL
    */
    void* GetGLProcAddressRaw(const char* name);

    // Load Xlib GLAD Functions based on unix, Xorg-based/X-To-Wayland-based host
    bool LoadGL();
private:
    Display* m_Display = nullptr;
    Window   m_Window  = 0;
    GLXContext m_Context = nullptr;
    int m_GLXMajor = 3, m_GLXMinor = 3;
    bool m_Initialized = false;
};

}
#endif