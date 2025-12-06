#pragma once
#include <cstring>
#ifdef __AX_GRAPHICS_GL__

#include <iostream>
#include <glad/glad.h>

#ifdef __AX_PLATFORM_X11__
#include <dlfcn.h> // dlsym
#include <GL/glx.h>
#endif

namespace axle::core::ctx::gl
{

// One-time libGL handle (or libOpenGL)
void* UnixX11OpenLibGLOnce();

/*
 * Low-level symbol resolver:
 *   1) Try glXGetProcAddressARB
 *   2) Fallback to dlsym on libGL or libOpenGL
 */
void* UnixX11GetGLProcAddressRaw(const char* name);

/*
 * Wrapper used by GLAD. We only log “critical” functions so we don’t drown in spam.
 * GLAD’s load fails if it can’t load glGetString / glGetIntegerv / glGetError / glGetStringi.
 */
void* UnixX11DebugGLLoader(const char* name);

// Load GLAD Functions based on unix, Xorg-based/X-To-Wayland-based host
bool UnixX11LoadGL();

// Should be called on post CTX::MakeCurrent()
inline bool LoadGLFunctions(void* contextLoader = nullptr) {
    if (contextLoader) {
        return gladLoadGLLoader((GLADloadproc)contextLoader);
    } else {
#if defined(_WIN32)
        // On Windows, we use wglGetProcAddress
        return gladLoadGLLoader((GLADloadproc)wglGetProcAddress);
#elif defined(__APPLE__)
        // macOS usually works directly
        return gladLoadGLLoader((GLADloadproc)aglGetProcAddress);
#elif defined(__AX_PLATFORM_X11__)
        // BSD / Linux / X11 / GLX (Yeah we deal w that)
        return UnixX11LoadGL();
    }
#else
    #error "GLAD loader not implemented for this platform"
#endif
}

}
#endif