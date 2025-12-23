#include "axle/core/app/AX_ApplicationWin32.hpp"
#include <mutex>

#ifdef __AX_PLATFORM_WIN32__

#include <AX_PCH.hpp>

#include <Windows.h>
#include <windowsx.h>

#include <stdexcept>

namespace axle::core {

static ApplicationWin32* g_App = nullptr; // global pointers for static WndProc (Window procedure)
static std::mutex g_AppMutex;

ApplicationWin32::ApplicationWin32(const ApplicationSpecification& spec) {
    if (g_App) throw std::runtime_error("Cannot have more than one application, maybe try asking microsoft to not call window events on window object construction, but just on intialization just like other opearting systems. I am lazy and not going to somehow magically overcome this");
    g_App = this;
    m_Title = spec.title;
    m_Width = spec.width;
    m_Height = spec.height;
    m_Resizable = spec.resizable;
    m_Executor.Init();
}

ApplicationWin32::~ApplicationWin32() {
    Shutdown();
    g_App = nullptr;
}

void ApplicationWin32::Launch() {
    if (m_Instance) return;
    m_Instance = GetModuleHandle(nullptr);

    // Register window class
    WNDCLASS wc = {};
    wc.lpfnWndProc = (WNDPROC)ApplicationWin32::WndProc;
    wc.hInstance = (HINSTANCE)m_Instance;
    wc.lpszClassName = "AXWin32WindowClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);

    DWORD style = WS_OVERLAPPEDWINDOW;
    if (!m_Resizable) style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);

    RECT rect = { 0, 0, (LONG)m_Width, (LONG)m_Height };
    AdjustWindowRect(&rect, style, FALSE);

    m_Hwnd = CreateWindowEx(
        0,
        wc.lpszClassName,
        m_Title.c_str(),
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr,
        nullptr,
        (HINSTANCE)m_Instance,
        nullptr
    );

    if (!m_Hwnd) {
        MessageBox(nullptr, "Failed to create window", "Error", MB_OK);
        exit(1);
    }

    ShowWindow((HWND)m_Hwnd, SW_SHOW);
}

void ApplicationWin32::Shutdown() {
    if (m_Hwnd) {
        DestroyWindow((HWND)m_Hwnd);
        m_Hwnd = nullptr;
    }
}

std::deque<Event> ApplicationWin32::PollEvents() {
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            m_ShouldQuit = true;
            return;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

}

bool ApplicationWin32::IsThrottling() {
    return !m_Focused || m_Grabbed;
}

void ApplicationWin32::SetTitle(const std::string& title) {
    m_Title = title;
    SetWindowText((HWND)m_Hwnd, m_Title.c_str());
}

void ApplicationWin32::SetResizable(bool enabled) {
    m_Resizable = enabled;

    LONG style = GetWindowLong((HWND)m_Hwnd, GWL_STYLE);

    if (enabled) {
        style |= (WS_THICKFRAME | WS_MAXIMIZEBOX);
    } else {
        style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    }

    SetWindowLong((HWND)m_Hwnd, GWL_STYLE, style);
    SetWindowPos((HWND)m_Hwnd, nullptr, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
}

void ApplicationWin32::SetCursorMode(CursorMode mode) {
    m_CursorMode = mode;

    if (mode == CursorMode::Normal) {
        ShowCursor(TRUE);
        ClipCursor(nullptr);
    }
    else if (mode == CursorMode::Hidden) {
        ShowCursor(FALSE);
        ClipCursor(nullptr);
    }
    else if (mode == CursorMode::Locked) {
        ShowCursor(FALSE);

        RECT rect;
        GetClientRect((HWND)m_Hwnd, &rect);
        POINT ul = { rect.left, rect.top };
        POINT lr = { rect.right, rect.bottom };
        ClientToScreen((HWND)m_Hwnd, &ul);
        ClientToScreen((HWND)m_Hwnd, &lr);
        rect = { ul.x, ul.y, lr.x, lr.y };

        ClipCursor(&rect);
    }
}

vLRESULT CALLBACK ApplicationWin32::WndProc(vHWND hwnd, vUINT msg, vWPARAM wParam, vLPARAM lParam)
{
    std::lock_guard<std::mutex> lock(g_AppMutex);
    ApplicationWin32* app = g_App;

    bool pressed;
    switch (msg)
    {
    case WM_CLOSE:
    {
        app->m_ShouldQuit = true;
        return 0;
    }
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        return 0;
    }
    case WM_SIZE:
    {
        uint32_t w = LOWORD(lParam);
        uint32_t h = HIWORD(lParam);
        app->m_Width = w;
        app->m_Height = h;

        if (app->m_ResizeCallback)
            app->m_ResizeCallback({ w, h });
        return 0;
    }
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
        pressed = (msg == WM_KEYDOWN);
        if (app->m_KeyCallback)
            app->m_KeyCallback({ (unsigned long)wParam, pressed });
        return 0;
    }
    case WM_MOUSEMOVE:
    {
        float x = (float)GET_X_LPARAM(lParam);
        float y = (float)GET_Y_LPARAM(lParam);

        if (app->m_MouseMoveCallback)
            app->m_MouseMoveCallback({ x, y });
        return 0;
    }
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    {
        pressed = (msg == WM_LBUTTONDOWN);
        if (pressed) {
           if (app->m_IsMouseOnEdge) app->m_Grabbed = true;
        } else {
            app->m_Grabbed = false;
        }
        if (app->m_MouseButtonCallback)
            app->m_MouseButtonCallback({ 0, pressed });
        return 0;
    }
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    {
        pressed = (msg == WM_RBUTTONDOWN);
        if (app->m_MouseButtonCallback)
            app->m_MouseButtonCallback({ 1, pressed });
        return 0;
    }
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    {
        pressed = (msg == WM_MBUTTONDOWN);
        if (app->m_MouseButtonCallback)
            app->m_MouseButtonCallback({ 2, pressed });
        return 0;
    }
    case WM_ACTIVATE:
    {
        app->m_Focused = (LOWORD(wParam) == WA_ACTIVE);
        if (app->m_FocusCallback)
            app->m_FocusCallback({app->m_Focused});
        return 0;
    }
    case WM_NCHITTEST:
    {
        LRESULT hit = DefWindowProc((HWND)hwnd, msg, wParam, lParam);

        if (hit == HTBOTTOM || hit == HTTOP || hit == HTRIGHT || hit == HTLEFT ||
            hit == HTBOTTOMLEFT || hit == HTBOTTOMRIGHT || hit == HTTOPLEFT || hit == HTTOPRIGHT) {
            app->m_IsMouseOnEdge = true;
        } else {
            app->m_IsMouseOnEdge = false;
        }
        return hit;
    }
    case WM_ENTERSIZEMOVE:
    case WM_EXITSIZEMOVE:
    {
        app->m_Grabbed = (msg == WM_ENTERSIZEMOVE);
        if (!app->m_Grabbed) app->m_IsMouseOnEdge = false;
        return 0;
    }
    }

    return DefWindowProc((HWND)hwnd, msg, wParam, lParam);
}

}
#endif