#if defined(__AX_PLATFORM_X11__) && defined(__AX_GRAPHICS_GL__)

#include "AX_PCH.hpp"

#include "axle/core/app/AX_ApplicationX11.hpp"
#include "axle/core/app/AX_IApplication.hpp"

#include "axle/core/ctx/GL/AX_GLRenderContextX11.hpp"

#include <X11/Xlib.h>

#include <glad/glad.h>
#include <GL/glx.h>

// Shared Objects Util
#include <dlfcn.h>

namespace axle::core {

GLRenderContextX11::GLRenderContextX11() = default;
GLRenderContextX11::~GLRenderContextX11() {
    Shutdown();
}

bool GLRenderContextX11::Init(IApplication* app) {
    if (m_Initialized) return true;

    ApplicationX11& appX11 = *(ApplicationX11*)(app);

    // nativeWindow is an X11 Window (Window) pointer and we need Display*
    // Expect the caller to pass an X11 Window as void* and be able to recover Display*
    // For convenience, we assume nativeWindow is actually a pointer-sized Window.
    // Caller should call GetDisplay() on the X11Application and ensure current X display.
    // Here: obtain default display (not ideal) but common in single-display apps.
    // We have to work on later for multiple Windows or Shared apps.
    m_Display = appX11.GetDisplay();
    if (!m_Display) {
        std::cerr << "AX Error: GL_RenderContext: XOpenDisplay failed\n";
        return false;
    }

    // get GLX version
    if (!glXQueryVersion(m_Display, &m_GLXMajor, &m_GLXMinor)) {
        std::cerr << "AX Error: GL_RenderContext: glXQueryVersion failed\n";
    }

    // Choose a visual/config
    // TODO: Make config available to user, so they can change it based on Client's Software & Hardware
    static int visual_attribs[] = {
        GLX_RENDER_TYPE,   GLX_RGBA_BIT,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_DOUBLEBUFFER, True,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        GLX_DEPTH_SIZE, 24,
        GLX_STENCIL_SIZE, 8,
        None
    };

    int fbcount = 0;
    GLXFBConfig* fbc = glXChooseFBConfig(m_Display, DefaultScreen(m_Display), visual_attribs, &fbcount);
    if (!fbc || fbcount == 0) {
        std::cerr << "AX Error: GL_RenderContext: glXChooseFBConfig failed\n";
        XCloseDisplay(m_Display);
        m_Display = nullptr;
        return false;
    }

    // pick first config
    GLXFBConfig chosenFBC = fbc[0];

    XVisualInfo* vi = glXGetVisualFromFBConfig(m_Display, chosenFBC);
    if (!vi) {
        std::cerr << "AX Error: GL_RenderContext: glXGetVisualFromFBConfig failed\n";
        XFree(fbc);
        XCloseDisplay(m_Display);
        m_Display = nullptr;
        return false;
    }

    appX11.DestroyWindow();
    appX11.CreateWindow(vi);

    m_Window = reinterpret_cast<Window>(appX11.GetNativeWindowHandle());

    // create a context
    // Try to use glXCreateContextAttribsARB for modern context if available
    typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
    glXCreateContextAttribsARBProc glXCreateContextAttribsARB = nullptr;
    glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)
        glXGetProcAddress(reinterpret_cast<const GLubyte*>("glXCreateContextAttribsARB"));

    if (glXCreateContextAttribsARB) {
        int context_attribs[] = {
            // Request a core profile 3.3
            GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
            GLX_CONTEXT_MINOR_VERSION_ARB, 3,
            // GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB, // maybe maybe, we try and findout
            None
        };
        m_Context = glXCreateContextAttribsARB(m_Display, chosenFBC, 0, True, context_attribs);
        // If creation failed, fall back
    }

    if (!m_Context) {
        m_Context = glXCreateNewContext(m_Display, chosenFBC, GLX_RGBA_TYPE, 0, True);
    }

    XFree(vi);
    XFree(fbc);

    if (!m_Context) {
        std::cerr << "AX Error: GL_RenderContext: failed to create GLX context\n";
        XCloseDisplay(m_Display);
        m_Display = nullptr;
        return false;
    }

    // Make current
    if (!glXMakeCurrent(m_Display, m_Window, m_Context)) {
        std::cerr << "AX Error: GL_RenderContext: glXMakeCurrent failed\n";
        glXDestroyContext(m_Display, m_Context);
        m_Context = nullptr;
        XCloseDisplay(m_Display);
        m_Display = nullptr;
        return false;
    }

    m_Initialized = true;
    return true;
}

void GLRenderContextX11::MakeCurrent() {
    if (m_Display && m_Context) {
        glXMakeCurrent(m_Display, m_Window, m_Context);
    }
}

void GLRenderContextX11::SwapBuffers() {
    if (m_Display) glXSwapBuffers(m_Display, m_Window);
}

void GLRenderContextX11::SetVSync(bool enabled) {
    // Try to use glXSwapIntervalEXT or glXSwapIntervalSGI if available
    // We'll attempt several methods; ignore failures.
    typedef void (*glXSwapIntervalEXTProc)(Display*, GLXDrawable, int);
    typedef int  (*glXSwapIntervalSGIProc)(int);
    typedef void (*glXSwapIntervalMESAProc)(unsigned int);

    // ext
    auto ext = (glXSwapIntervalEXTProc)glXGetProcAddress((const GLubyte*)"glXSwapIntervalEXT");
    if (ext) {
        ext(m_Display, m_Window, enabled ? 1 : 0);
        return;
    }

    // sgi
    auto sgi = (glXSwapIntervalSGIProc)glXGetProcAddress((const GLubyte*)"glXSwapIntervalSGI");
    if (sgi) {
        sgi(enabled ? 1 : 0);
        return;
    }

    // mesa
    auto mesa = (glXSwapIntervalMESAProc)glXGetProcAddress((const GLubyte*)"glXSwapIntervalMESA");
    if (mesa) {
        mesa(enabled ? 1u : 0u);
        return;
    }
}

void GLRenderContextX11::Shutdown() {
    if (!m_Initialized) return;

    if (m_Display && m_Context) {
        glXMakeCurrent(m_Display, None, nullptr);
        glXDestroyContext(m_Display, m_Context);
        m_Context = nullptr;
    }

    m_Window = 0;
    m_Initialized = false;
}

// ret ctx
void* GLRenderContextX11::GetContextHandle() const {
    return reinterpret_cast<void*>(m_Context);
}

// One-time libGL handle (or libOpenGL)
void* OpenLibGLOnce() {
    static void* handle = nullptr;
    static bool tried = false;

    if (tried) return handle;
    tried = true;

    // All BSD, Linux, ... Based, OpenGL shared-lib names
    const char* candidates[] = {
        "libOpenGL.so.0",  // GLVND style
        "libGL.so.1", // classic
        "libGL.so" // fallback
    };
    for (const char* name : candidates) {
        handle = dlopen(name, RTLD_LAZY | RTLD_LOCAL);
        if (handle) {
            return handle;
        } else {
            std::cerr << "AX Error: OpenLibGLOnce() dlopen(" << name << ") failed: "
                      << (dlerror() ? dlerror() : "(no dlerror)") << "\n";
        }
    }

    std::cerr << "AX Error: OpenLibGLOnce() Could not open any GL library.\n";
    return nullptr;
}

void* GetGLProcAddressRaw(const char* name) {
    // 1) glXGetProcAddressARB
    auto p1 = (void*)glXGetProcAddressARB((const GLubyte*)name);
    if (p1) return p1;

    // 2) dlsym
    void* lib = OpenLibGLOnce();
    if (!lib) {
        std::cerr << "AX Error: GetGLProcAddrRaw() no libGL handle for " << name << "\n";
        return nullptr;
    }

    void* p2 = dlsym(lib, name);
    if (!p2) {
        // This is especially important for core functions like glGetString/glGetIntegerv
        std::cerr << "AX Error: GetGLProcAddrRaw() dlsym(" << name << ") failed: "
                  << (dlerror() ? dlerror() : "(no dlerror)") << "\n";
    }
    return p2;
}

// Add Debugger/Debug builds, Logger, cleaner debug management
bool GLRenderContextX11::LoadGLFunctions() {
    return gladLoadGLLoader((GLADloadproc)GetGLProcAddressRaw);
}

}
#endif
