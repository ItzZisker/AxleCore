#include "axle/core/app/AX_ApplicationWin32.hpp"
#include "axle/core/app/AX_IApplication.hpp"
#include <iostream>
#include <ostream>

#ifdef __AX_PLATFORM_WIN32__

#include <AX_PCH.hpp>

#include <Windows.h>
#include <windowsx.h>

#include <stdexcept>

namespace axle::core {

static ApplicationWin32* g_App = nullptr; // global pointers for static WndProc (Window procedure)

ApplicationWin32::ApplicationWin32(const ApplicationSpecification& spec, uint32_t maxSharedEvents)
    : IApplication(spec, maxSharedEvents) {
    if (g_App) throw std::runtime_error("Cannot have more than one application, maybe try asking microsoft to not call window events on window object construction, but just on intialization just like other opearting systems. I am lazy and not going to somehow magically overcome this");
    g_App = this;
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
    wc.lpszClassName = "Application WIN32";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);

    DWORD style = WS_OVERLAPPEDWINDOW;
    if (!m_State.IsResizable()) style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);

    RECT rect = { 0, 0, (LONG)m_State.GetWidth(), (LONG)m_State.GetHeight() };
    AdjustWindowRect(&rect, style, FALSE);

    m_Hwnd = CreateWindowEx(
        0,
        wc.lpszClassName,
        m_State.GetTitle().c_str(),
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

void ApplicationWin32::PollEvents() {
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            m_State.RequestQuit();
        } else {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

void ApplicationWin32::SetTitle(const std::string& title) {
    SetWindowText((HWND)m_Hwnd, title.c_str());
    m_State.SetTitle(title);
}

void ApplicationWin32::SetResizable(bool enabled) {
    LONG style = GetWindowLong((HWND)m_Hwnd, GWL_STYLE);

    if (enabled) {
        style |= (WS_THICKFRAME | WS_MAXIMIZEBOX);
    } else {
        style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    }

    SetWindowLong((HWND)m_Hwnd, GWL_STYLE, style);
    SetWindowPos((HWND)m_Hwnd, nullptr, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
    
    m_State.SetResizable(enabled);
}

void ApplicationWin32::SetCursorMode(CursorMode mode) {
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
    m_State.SetCursorMode(mode);
}

vLRESULT CALLBACK ApplicationWin32::WndProc(vHWND hwnd, vUINT msg, vWPARAM wParam, vLPARAM lParam)
{
    ApplicationWin32* app = g_App;

    Event event;
    switch (msg)
    {
    case WM_CLOSE:
    {
        app->m_State.RequestQuit();
        break;
    }
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        break;
    }
    case WM_SIZE:
    {
        uint32_t w = LOWORD(lParam);
        uint32_t h = HIWORD(lParam);
        app->m_State.SetSize(w, h);

        event.type = EventType::WindowResize;
        event.value.windowResize = { w, h };
        break;
    }
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
        event.type = EventType::Key;
        event.value.key = { (uint64_t)wParam, (msg == WM_KEYDOWN) };
        break;
    }
    case WM_MOUSEMOVE:
    {
        float x = (float)GET_X_LPARAM(lParam);
        float y = (float)GET_Y_LPARAM(lParam);

        event.type = EventType::MouseMove;
        event.value.mouseMove = { x, y };
        break;
    }
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    {
        bool pressed = (msg == WM_LBUTTONDOWN);
        if (pressed) {
           if (app->m_IsMouseOnEdge) app->m_Grabbed = true;
        } else {
            app->m_Grabbed = false;
        }
        event.type = EventType::MouseButton;
        event.value.mouseButton = { 0, pressed };
        break;
    }
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    {
        event.type = EventType::MouseButton;
        event.value.mouseButton = { 1, (msg == WM_RBUTTONDOWN) };
        break;
    }
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    {
        event.type = EventType::MouseButton;
        event.value.mouseButton = { 2, (msg == WM_MBUTTONDOWN) };
        break;
    }
    case WM_ACTIVATE:
    {
        app->m_Focused = (LOWORD(wParam) == WA_ACTIVE);
        event.type = EventType::WindowFocus;
        event.value.windowFocus = { app->m_Focused };
        break;
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
        break;
    }
    }

    if (event.type != EventType::Void) {
        app->m_State.PushEvent(event);
        return 0;
    }

    return DefWindowProc((HWND)hwnd, msg, wParam, lParam);
}

}
#endif