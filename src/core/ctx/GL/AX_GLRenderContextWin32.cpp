#include "axle/core/ctx/GL/AX_GLRenderContextWin32.hpp"
#include <mutex>

#if defined(__AX_GRAPHICS_GL__) && defined(_WIN32) && defined(__AX_PLATFORM_WIN32__)
#include <windows.h>
#include <gl/GL.h>

#include <iostream>

namespace axle::core {

GLRenderContextWin32::GLRenderContextWin32() = default;
GLRenderContextWin32::~GLRenderContextWin32() {
    Shutdown();
}

bool GLRenderContextWin32::Init(IApplication* app) {
    if (m_Initialized) return true;
    if (!app) return false;

    std::lock_guard<std::mutex> lock(app->m_HandleMutex);

    m_hwnd = reinterpret_cast<HWND>(app->GetNativeWindowHandle());

    // 1. Acquire device context
    // What the hell is DC? why you can't just call it GetDisplayContext?
    // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getdc
    // Oh god, without internet, writing windows specific code is Impossible
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

void GLRenderContextWin32::MakeCurrent() {
    if (m_hdc && m_hglrc) {
        wglMakeCurrent((HDC)m_hdc, (HGLRC)m_hglrc);
    }
}

void GLRenderContextWin32::SwapBuffers() {
    if (m_hdc) ::SwapBuffers((HDC)m_hdc);
}

void GLRenderContextWin32::SetVSync(bool enabled) {
    typedef BOOL(WINAPI *PFNWGLSWAPINTERVALEXTPROC)(int);
    static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = nullptr;

    if (!wglSwapIntervalEXT) {
        wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
    }
    if (wglSwapIntervalEXT) {
        wglSwapIntervalEXT(enabled ? 1 : 0);
    }
}

void GLRenderContextWin32::Shutdown() {
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

void* GLRenderContextWin32::GetContextHandle() const {
    return reinterpret_cast<void*>(m_hglrc);
}

void* GetGLProcAddressRaw(const char* name) {
    void* p = (void*)wglGetProcAddress(name);
    if (p) return p;
    static HMODULE mod = LoadLibraryA("opengl32.dll");
    if (!mod) return nullptr;
    return (void*)GetProcAddress(mod, name);
}

bool GLRenderContextWin32::LoadGLFunctions() {
    return gladLoadGLLoader((GLADloadproc)GetGLProcAddressRaw);
}

}
#endif
