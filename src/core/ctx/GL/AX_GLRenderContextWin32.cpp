#include "axle/core/ctx/GL/AX_GL_RenderContextWin32.hpp"

#include "AX_PCH.hpp"

#if defined(__AX_GRAPHICS_GL__) && defined(_WIN32) && defined(__AX_PLATFORM_WIN32__)
#include <windows.h>
#include <gl/GL.h>

namespace axle::core {

GL_RenderContextWin32::GL_RenderContextWin32() = default;
GL_RenderContextWin32::~GL_RenderContextWin32() {
    Shutdown();
}

bool GL_RenderContextWin32::Init(IApplication* app) {
    if (m_Initialized) return true;

    m_hwnd = reinterpret_cast<HWND>(app->GetNativeWindowHandle());

    // 1. Acquire device context
    m_hdc = GetDC((HWND)m_hwnd);
    if (!m_hdc) {
        std::cerr << "AX Error: Failed to GetDC()\n";
        return false;
    }

    // TODO: add cross-platform config for GL context for each application implementation
    // 2. Set pixel format
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags =
        PFD_DRAW_TO_WINDOW |
        PFD_SUPPORT_OPENGL |
        PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pixelFormat = ChoosePixelFormat((HDC)m_hdc, &pfd);
    if (!pixelFormat) {
        std::cerr << "AX Error: ChoosePixelFormat failed!\n";
        return false;
    }
    if (!SetPixelFormat((HDC)m_hdc, pixelFormat, &pfd)) {
        std::cerr << "AX Error: SetPixelFormat failed!\n";
        return false;
    }

    // 3. Create OpenGL context
    m_hglrc = wglCreateContext((HDC)m_hdc);
    if (!m_hglrc) {
        std::cerr << "AX Error: wglCreateContext failed!\n";
        return false;
    }

    // 4. Make it current
    if (!wglMakeCurrent((HDC)m_hdc, (HGLRC)m_hglrc)) {
        std::cerr << "AX Error: wglMakeCurrent failed!\n";
        wglDeleteContext((HGLRC)m_hglrc);
        m_hglrc = nullptr;
        return false;
    }

    // 5. Oh god please, Microsoft Refactor these names or add wrappers atleast;
    //    if there were no GPT, it would be a horrible experience just to write above lines

    m_Initialized = true;
    return true;
}

void GL_RenderContextWin32::MakeCurrent() {
    if (m_hdc && m_hglrc) {
        wglMakeCurrent((HDC)m_hdc, (HGLRC)m_hglrc);
    }
}

void GL_RenderContextWin32::SwapBuffers() {
    if (m_hdc) ::SwapBuffers((HDC)m_hdc);
}

void GL_RenderContextWin32::SetVSync(bool enabled) {
    typedef BOOL(WINAPI *PFNWGLSWAPINTERVALEXTPROC)(int);
    static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = nullptr;

    if (!wglSwapIntervalEXT) {
        wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
    }
    if (wglSwapIntervalEXT) {
        wglSwapIntervalEXT(enabled ? 1 : 0);
    }
}

void GL_RenderContextWin32::Shutdown() {
    if (!m_Initialized) return;

    if (m_hglrc) {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext((HGLRC)m_hglrc);
        m_hglrc = nullptr;
    }
    if (m_hdc) {
        ReleaseDC((HWND)m_hwnd, (HDC)m_hdc);
        m_hdc = nullptr;
    }

    m_hwnd = nullptr;
    m_Initialized = false;
}

void* GL_RenderContextWin32::GetContextHandle() const {
    return reinterpret_cast<void*>(m_hglrc);
}

bool GL_RenderContextWin32::LoadGLFunctions() {
    return gladLoadGLLoader((GLADloadproc)wglGetProcAddress);
}

}
#endif
