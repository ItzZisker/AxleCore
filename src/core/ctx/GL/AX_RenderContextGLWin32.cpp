#include "axle/core/ctx/GL/AX_RenderContextGLWin32.hpp"

#if defined(__AX_GRAPHICS_GL__) && defined(_WIN32) && defined(__AX_PLATFORM_WIN32__)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifdef AX_DEBUG
#define wglMakeCurrent(hdc, hglrc) \
    [&](){ \
        auto result = ::wglMakeCurrent(hdc, hglrc); \
        std::cout << "wglMakeCurrent called with HDC=" << hdc << " HGLRC=" << hglrc \
                  << ", result=" << result \
                  << ", curHDC=" << ::wglGetCurrentDC() \
                  << ", curHGLRC=" << ::wglGetCurrentContext() \
                  << ", GetLastError=" << GetLastError() << "\n"; \
        return result; \
    }()
#endif

#include <glad/gl.h>

#include <memory>
#include <mutex>

#include <string>
#include <iostream>

namespace axle::core {

typedef BOOL (WINAPI *PFNWGLSWAPINTERVALEXTPROC)(int);
typedef HGLRC (GLAD_API_PTR *PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC hDC, HGLRC hShareContext, const int * attribList);

RenderContextGLWin32::RenderContextGLWin32() {}
RenderContextGLWin32::~RenderContextGLWin32() { Shutdown(); }

void* GetGLProcAddressRaw(const char* name) {
    void* p = (void*)wglGetProcAddress(name);
    if (p) return p;
    static HMODULE mod = LoadLibraryA("opengl32.dll");
    if (!mod) return nullptr;
    return (void*)GetProcAddress(mod, name);
}

bool RenderContextGLWin32::Init(SharedPtr<IWindow> window) {
    if (m_Initialized) return true;
    if (!window) return false;

    std::lock_guard<std::mutex> lock(window->m_HandleMutex);
    m_hwnd = reinterpret_cast<HWND>(window->GetNativeWindowHandle());
    m_hdc = GetDC((HWND)m_hwnd);

    if (!m_hdc) return false;

    RECT rect;
    GetClientRect((HWND)m_hwnd, &rect);
    m_Handle->surfaceInfo.width  = rect.right  - rect.left;
    m_Handle->surfaceInfo.height = rect.bottom - rect.top;

    // TODO: Make OpenGL pfd configurable
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = (BYTE)m_Handle->surfaceInfo.colorBits;
    pfd.cDepthBits = (BYTE)m_Handle->surfaceInfo.depthBits;
    pfd.cStencilBits = (BYTE)m_Handle->surfaceInfo.stenctilBits;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pixelFormat = ChoosePixelFormat((HDC)m_hdc, &pfd);
    if (!pixelFormat) return false;
    if (!SetPixelFormat((HDC)m_hdc, pixelFormat, &pfd)) return false;

    // Create dummy context
    HGLRC dummyContext = wglCreateContext((HDC)m_hdc);
    if (!dummyContext) return false;

    if (!wglMakeCurrent((HDC)m_hdc, dummyContext)) {
        wglDeleteContext(dummyContext);
        return false;
    }

    // Load modern context creation function
    if (!m_Func_wglCreateContextAttribsARB) {
        auto wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC) wglGetProcAddress("wglCreateContextAttribsARB");
        m_Func_wglCreateContextAttribsARB = wglCreateContextAttribsARB;
    }

    auto wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC) m_Func_wglCreateContextAttribsARB;
    if (!wglCreateContextAttribsARB) {
        // No modern context support
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(dummyContext);
        return false;
    }

    // Try highest → lowest versions
    struct Version { int major; int minor; };

    Version versions[] = {
        {4,6}, {4,5}, {4,4}, {4,3},
        {4,2}, {4,1}, {4,0},
        {3,3}
    };

    HGLRC realContext{nullptr};

    for (const auto& v : versions) {
        int attribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, v.major,
            WGL_CONTEXT_MINOR_VERSION_ARB, v.minor,
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
#ifdef _DEBUG
            WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
            0
        };

        realContext = wglCreateContextAttribsARB((HDC)m_hdc, 0, attribs);
        if (realContext) break; // Break finally when we got a supported version
    }

    // Cleanup dummy
    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(dummyContext);

    if (!realContext)
        return false;

    // Activate real context
    if (!wglMakeCurrent((HDC)m_hdc, realContext)) {
        wglDeleteContext(realContext);
        return false;
    }

    m_Handle->hglrc = realContext;

    // Load GL functions
    if (!gladLoaderLoadGLContext(&m_Handle->glCtx)) {
        return false;
    }

    // Verify version ≥ 3.3
    int major = 0, minor = 0;
    m_Handle->glCtx.GetIntegerv(GL_MAJOR_VERSION, &major);
    m_Handle->glCtx.GetIntegerv(GL_MINOR_VERSION, &minor);

    if (major < 3 || (major == 3 && minor < 3))
        return false;

    m_Initialized = true;
    return true;
}

void RenderContextGLWin32::MakeCurrent() {
    if (m_hdc && m_Handle->hglrc) {
        wglMakeCurrent((HDC)m_hdc, (HGLRC)m_Handle->hglrc);
    }
}

void RenderContextGLWin32::ReleaseCurrent() {
    wglMakeCurrent(nullptr, nullptr);
}

void RenderContextGLWin32::SwapBuffers() {
    if (m_hdc) ::SwapBuffers((HDC)m_hdc);
}

void RenderContextGLWin32::SetVSync(bool enabled) {
    if (!m_Func_wglSwapIntervalEXT) {
        m_Func_wglSwapIntervalEXT = wglGetProcAddress("wglSwapIntervalEXT");
    }
    auto wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC) m_Func_wglSwapIntervalEXT;
    if (wglSwapIntervalEXT) {
        wglSwapIntervalEXT(enabled ? 1 : 0);
    }
}

void RenderContextGLWin32::Shutdown() {
    if (!m_Initialized) return;

    if (m_Handle->hglrc) {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext((HGLRC)m_Handle->hglrc);
        m_Handle->hglrc = nullptr;
    }
    if (m_hdc) {
        ReleaseDC((HWND)m_hwnd, (HDC)m_hdc);
        m_hdc = nullptr;
    }

    m_hwnd = nullptr;
    m_Initialized = false;
}

SharedPtr<void> RenderContextGLWin32::GetContextHandle() const {
    return (SharedPtr<void>)m_Handle;
}

}
#endif
