#pragma once

#include "axle/core/app/AX_IApplication.hpp"
#if defined(__AX_PLATFORM_X11__) && defined(__AX_GRAPHICS_GL__)
#include "../AX_IRenderContext.hpp"

#include <X11/Xlib.h>

struct __GLXcontextRec;
using GLXContext = struct __GLXcontextRec*;

namespace axle::core {

class GLRenderContextX11 : public IRenderContext {
public:
    GLRenderContextX11();
    virtual ~GLRenderContextX11();

    bool Init(IApplication* app) override;
    void MakeCurrent();
    void SwapBuffers() override;
    void SetVSync(bool enabled) override;
    void Shutdown() override;

    void* GetContextHandle() const override;

    // Define GLAD/glx 3.3 Functions, must be called post Context Creation.
    // Returns true if success, false if none or atleast one of functions failed to load.
    // Xlib GLAD Functions based on unix, Xorg-based/X-To-Wayland-based host
    bool LoadGLFunctions();
private:
    Display* m_Display = nullptr;
    Window   m_Window  = 0;
    GLXContext m_Context = nullptr;
    int m_GLXMajor = 3, m_GLXMinor = 3;
    bool m_Initialized = false;
};

}
#endif