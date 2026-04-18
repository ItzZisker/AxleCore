#pragma once

#if defined(__AX_PLATFORM_X11__) && defined(__AX_GRAPHICS_GL__)
#include "../AX_IRenderContextGL.hpp"

#include <X11/Xlib.h>

struct __GLXcontextRec;
using GLXContext = struct __GLXcontextRec*;

namespace axle::core {

class RenderContextGLX11 : public IRenderContextGL {
public:
    RenderContextGLX11();
    virtual ~GLRenderContextX11();

    bool Init(IWindow* app) override;
    void MakeCurrent();
    void SwapBuffers() override;
    void SetVSync(bool enabled) override;
    void Shutdown() override;

    SharedPtr<void> GetContextHandle() const override;

    // Define GLAD/glx 3.3 Functions, must be called post Context Creation.
    // Returns true if success, false if none or atleast one of functions failed to load.
    // Xlib GLAD Functions based on unix, Xorg-based/X-To-Wayland-based host
    bool LoadGLFunctions() override;
private:
    Display* m_Display = nullptr;
    Window   m_Window  = 0;
    GLXContext m_Context = nullptr;
    int m_GLXMajor = 3, m_GLXMinor = 3;
    bool m_Initialized = false;
};

}
#endif