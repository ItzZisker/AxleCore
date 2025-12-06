#include "axle/core/ctx/GL/AX_GL_RenderContext.hpp"

namespace axle::core::ctx::gl
{

// One-time libGL handle (or libOpenGL)
void* UnixX11OpenLibGLOnce() {
    static void* handle = nullptr;
    static bool tried = false;

    if (tried) return handle;
    tried = true;

    // All BSD, Linux, ... Based, OpenGL shared-lib names
    const char* candidates[] = {
        "libOpenGL.so.0",  // GLVND style
        "libGL.so.1",      // classic
        "libGL.so"         // fallback
    };
    for (const char* name : candidates) {
        handle = dlopen(name, RTLD_LAZY | RTLD_LOCAL);
        if (handle) {
            std::cerr << "[DBG] dlopen(" << name << ") OK, handle=" << handle << "\n";
            return handle;
        } else {
            std::cerr << "[DBG] dlopen(" << name << ") failed: "
                      << (dlerror() ? dlerror() : "(no dlerror)") << "\n";
        }
    }

    std::cerr << "[DBG] Could not open any GL library.\n";
    return nullptr;
}


void* UnixX11GetGLProcAddressRaw(const char* name) {
    // 1) glXGetProcAddressARB
    auto p1 = (void*)glXGetProcAddressARB((const GLubyte*)name);
    if (p1) {
        // std::cerr << "[DBG] glXGetProcAddressARB(" << name << ") = " << p1 << "\n";
        return p1;
    }

    // 2) dlsym
    void* lib = UnixX11OpenLibGLOnce();
    if (!lib) {
        std::cerr << "[DBG] GetGLProcAddressRaw: no libGL handle for " << name << "\n";
        return nullptr;
    }

    void* p2 = dlsym(lib, name);
    if (!p2) {
        // This is especially important for core functions like glGetString/glGetIntegerv
        std::cerr << "[DBG] dlsym(" << name << ") failed: "
                  << (dlerror() ? dlerror() : "(no dlerror)") << "\n";
    } else {
        std::cerr << "[DBG] dlsym(" << name << ") = " << p2 << "\n";
    }
    return p2;
}

void* UnixX11DebugGLLoader(const char* name) {
    void* p = UnixX11GetGLProcAddressRaw(name);

    if (!p &&
        (strcmp(name, "glGetString")   == 0 ||
         strcmp(name, "glGetIntegerv") == 0 ||
         strcmp(name, "glGetError")    == 0 ||
         strcmp(name, "glGetStringi")  == 0)) {
        std::cerr << "[GLAD-DBG] FAILED to load critical symbol: " << name << "\n";
    }

    return p;
}

bool UnixX11LoadGL() {
    std::cerr << "[DBG] Calling gladLoadGLLoader(DebugGLLoader) ...\n";
    int ok = gladLoadGLLoader((GLADloadproc)UnixX11DebugGLLoader);
    std::cerr << "[DBG] gladLoadGLLoader returned: " << ok << "\n";

    if (!ok) {
        std::cerr << "[DBG] GLAD failed to initialize. Critical symbol(s) above should hint why.\n";
        return false;
    }

    int major = 0, minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    std::cerr << "[DBG] GL reported version: " << major << "." << minor << "\n";

    const unsigned char* vendor   = glGetString(GL_VENDOR);
    const unsigned char* renderer = glGetString(GL_RENDERER);
    std::cerr << "[DBG] GL_VENDOR  : " << (vendor   ? (const char*)vendor   : "<null>") << "\n";
    std::cerr << "[DBG] GL_RENDERER: " << (renderer ? (const char*)renderer : "<null>") << "\n";

    return true;
}

}