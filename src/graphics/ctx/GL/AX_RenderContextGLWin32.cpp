#if defined(__AX_GRAPHICS_GL__) && defined(_WIN32) && defined(__AX_PLATFORM_WIN32__)

#include "axle/graphics/ctx/GL/AX_RenderContextGLWin32.hpp"
#include "axle/core/window/AX_WindowWin32.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <glad/gl.h>
#include <glad/wgl.h>

#include <memory>
#include <mutex>

#include <string>
#include <iostream>
#include <stdexcept>

namespace axle::gfx {

RenderContextGLWin32::RenderContextGLWin32() {}
RenderContextGLWin32::~RenderContextGLWin32() { Shutdown(); }

void* GetGLProcAddressRaw(const char* name) {
    void* p = (void*)wglGetProcAddress(name);
    if (p) return p;
    static HMODULE mod = LoadLibraryA("opengl32.dll");
    if (!mod) return nullptr;
    return (void*)GetProcAddress(mod, name);
}

LRESULT CALLBACK DummyWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

utils::ExError RenderContextGLWin32::Init(SharedPtr<core::IWindow> window, SurfaceDesc desc) {
    if (m_Initialized) return {"Already initialized"};
    if (!window) return {"window is nullptr / destroyed"};

    m_Handle->surfaceInfo = ToGLCSurfaceInfo(desc);

    std::lock_guard<std::mutex> lock(window->m_HandleMutex);
    m_hwnd = reinterpret_cast<HWND>(window->GetNativeWindowHandle());
    m_hdc = GetDC((HWND)m_hwnd);

    if (!m_hdc) return {"Cannot fetch main window's display"};

    RECT rect;
    GetClientRect((HWND)m_hwnd, &rect);
    m_Handle->surfaceInfo.width  = rect.right  - rect.left;
    m_Handle->surfaceInfo.height = rect.bottom - rect.top;

    WNDCLASS dummyWc = {};
    dummyWc.lpfnWndProc = (WNDPROC)DummyWndProc;
    dummyWc.hInstance = (HINSTANCE)GetModuleHandle(nullptr);
    dummyWc.lpszClassName = "Dummy Window for PixelFormatDecriptor";
    dummyWc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&dummyWc);

    m_dummyhwnd = CreateWindowEx(
        0,
        dummyWc.lpszClassName,
        dummyWc.lpszClassName,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr,
        nullptr,
        (HINSTANCE)dummyWc.hInstance,
        nullptr
    );
    if (!m_dummyhwnd) return {"Cannot create dummy window handle"};

    m_dummyhdc = GetDC((HWND)m_dummyhwnd);
    if (!m_dummyhdc) return {"Cannot fetch dummy window handle's display context"};

    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = (BYTE)m_Handle->surfaceInfo.colorBits;
    pfd.cAlphaBits = (BYTE)m_Handle->surfaceInfo.alphaBits;
    pfd.cDepthBits = (BYTE)m_Handle->surfaceInfo.depthBits;
    pfd.cStencilBits = (BYTE)m_Handle->surfaceInfo.stencilBits;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pixelFormat = ChoosePixelFormat((HDC)m_dummyhdc, &pfd);
    if (!pixelFormat) return {"Cannot choose pixelformat for dummy window display"};
    if (!SetPixelFormat((HDC)m_dummyhdc, pixelFormat, &pfd)) return {"Cannot SetPixelformat for dummy window display"};

    // Create dummy context
    HGLRC dummyContext = wglCreateContext((HDC)m_dummyhdc);
    if (!dummyContext) return {"Cannot create dummy context"};

    if (!wglMakeCurrent((HDC)m_dummyhdc, dummyContext)) {
        wglDeleteContext(dummyContext);
        return {"Cannot make dummy context as current context"};
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
        return {"Cannot fetch wgl function \"wglCreateContextAttribsARB\""};
    }

    // Load modern pixel format selection function
    if (!m_Func_wglChoosePixelFormatARB) {
        auto wglCreateContextAttribsARB = (PFNWGLCHOOSEPIXELFORMATARBPROC) wglGetProcAddress("wglChoosePixelFormatARB");
        m_Func_wglChoosePixelFormatARB = wglCreateContextAttribsARB;
    }

    auto wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC) m_Func_wglChoosePixelFormatARB;
    if (!wglChoosePixelFormatARB) {
        // No modern pixel format selection support
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(dummyContext);
        return {"Cannot fetch wgl function \"wglChoosePixelFormatARB\""};
    }
    
    std::vector<int> pixelAttribs;
    pixelAttribs.push_back(WGL_DRAW_TO_WINDOW_ARB);
    pixelAttribs.push_back(GL_TRUE);

    pixelAttribs.push_back(WGL_SUPPORT_OPENGL_ARB);
    pixelAttribs.push_back(GL_TRUE);

    pixelAttribs.push_back(WGL_DOUBLE_BUFFER_ARB);
    pixelAttribs.push_back(GL_TRUE);

    pixelAttribs.push_back(WGL_PIXEL_TYPE_ARB);
    pixelAttribs.push_back(WGL_TYPE_RGBA_ARB);

    pixelAttribs.push_back(WGL_COLOR_BITS_ARB);
    pixelAttribs.push_back(m_Handle->surfaceInfo.colorBits);

    if (m_Handle->surfaceInfo.alphaBits > 0) {
        pixelAttribs.push_back(WGL_ALPHA_BITS_ARB);
        pixelAttribs.push_back(m_Handle->surfaceInfo.alphaBits);
    }

    pixelAttribs.push_back(WGL_DEPTH_BITS_ARB);
    pixelAttribs.push_back(m_Handle->surfaceInfo.depthBits);

    if (m_Handle->surfaceInfo.stencilBits > 0) {
        pixelAttribs.push_back(WGL_STENCIL_BITS_ARB);
        pixelAttribs.push_back(m_Handle->surfaceInfo.stencilBits);
    }

    auto samples = m_Handle->surfaceInfo.samples;

    if (samples > 1) {
        pixelAttribs.push_back(WGL_SAMPLE_BUFFERS_ARB);
        pixelAttribs.push_back(1);

        pixelAttribs.push_back(WGL_SAMPLES_ARB);
        pixelAttribs.push_back(samples);
    }

    pixelAttribs.push_back(0);

    int pixelFormatARB;
    UINT numFormatsARB;
    wglChoosePixelFormatARB((HDC)m_hdc, pixelAttribs.data(), NULL, 1, &pixelFormatARB, &numFormatsARB);

    if (numFormatsARB < 1) {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(dummyContext);
        return {"Cannot create pixel format \"wglChoosePixelFormatARB\", double-check attributes"};
    }

    DescribePixelFormat((HDC)m_hdc, pixelFormatARB, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
    SetPixelFormat((HDC)m_hdc, pixelFormatARB, &pfd);

    // Try highest → lowest versions
    struct Version { int major; int minor; };

    Version versions[] = {
        {4,6}, {4,5}, {4,4}, {4,3},
        {4,2}, {4,1}, {4,0},
        {3,3}
    };

    HGLRC realContext{nullptr};

    for (const auto& v : versions) {
        int contextAttribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, v.major,
            WGL_CONTEXT_MINOR_VERSION_ARB, v.minor,
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,  
#ifdef _DEBUG
            WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
            0
        };

        realContext = wglCreateContextAttribsARB((HDC)m_hdc, 0, contextAttribs);
        if (realContext) break; // Break finally when we got a supported version
    }

    // Cleanup dummy
    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(dummyContext);
    DestroyWindow((HWND)m_dummyhwnd);
    UnregisterClass(dummyWc.lpszClassName, dummyWc.hInstance);
    m_dummyhwnd = m_dummyhdc = dummyContext = nullptr;

    if (!realContext) {
        return {"Context creation failed, OpenGL 3.3 either not supported or some internal errors occurred."};
    }

    // Activate real context
    if (!wglMakeCurrent((HDC)m_hdc, realContext)) {
        wglDeleteContext(realContext);
        return {"Cannot make context as current context"};
    }

    m_Handle->hglrc = realContext;

    // Load GL functions
    if (!gladLoaderLoadGLContext(&m_Handle->glCtx)) {
        return {"Cannot load GL functions (GLAD)"};
    }

    // Verify version ≥ 3.3
    int major = 0, minor = 0;
    m_Handle->glCtx.GetIntegerv(GL_MAJOR_VERSION, &major);
    m_Handle->glCtx.GetIntegerv(GL_MINOR_VERSION, &minor);

    if (major < 3 || (major == 3 && minor < 3)) {
        m_Handle->hglrc = nullptr;
        m_Handle->glCtx = {nullptr};
        wglDeleteContext(realContext);
        return {"Context version verification failed: major=" + std::to_string(major) + ", minor=" + std::to_string(minor)};
    }

    m_Initialized = true;
    return utils::ExError::NoError();
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
    if (m_Func_wglSwapIntervalEXT) {
        auto wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC) m_Func_wglSwapIntervalEXT;
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

GLCSurfaceInfo ToGLCSurfaceInfo(SurfaceDesc desc) {
    GLCSurfaceInfo res;
    res.width = desc.width;
    res.height = desc.height;
    res.alphaBits = GetAlphaBits(desc.colors);
    res.colorBits = GetColorBits(desc.colors);
    res.depthBits = GetDepthBits(desc.depthStencil);
    res.stencilBits = GetStencilBits(desc.depthStencil);
    res.samples = GetSampleCount(desc.samples);
    res.vsync = desc.vsync;
    return res;
}

}
#endif
