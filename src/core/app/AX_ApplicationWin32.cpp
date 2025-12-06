#include "axle/core/app/AX_ApplicationWin32.hpp"

#if defined(_WIN32) && defined(__AX_PLATFORM_WIN32__)

#include "AX_CorePCH.h"

#include <Windows.h>
#include <windowsx.h>

namespace axle
{
static ApplicationWin32* g_App = nullptr; // global pointer for static WndProc (Window procedure)

ApplicationWin32::ApplicationWin32(const ApplicationSpecification& spec) {
    g_App = this;

    m_Title = spec.title;
    m_Width = spec.width;
    m_Height = spec.height;
    m_Resizable = spec.resizable;
}

ApplicationWin32::~ApplicationWin32() {
    Shutdown();
}

void ApplicationWin32::Launch() {
    m_Instance = GetModuleHandle(nullptr);

    // Register window class
    WNDCLASS wc = {};
    wc.lpfnWndProc = ApplicationWin32::WndProc;
    wc.hInstance = m_Instance;
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
        m_Instance,
        nullptr
    );

    if (!m_Hwnd) {
        MessageBox(nullptr, "Failed to create window", "Error", MB_OK);
        exit(1);
    }

    ShowWindow(m_Hwnd, SW_SHOW);
}

void ApplicationWin32::Shutdown() {
    if (m_Hwnd) {
        DestroyWindow(m_Hwnd);
        m_Hwnd = nullptr;
    }
}

void ApplicationWin32::PollEvents() {
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

void ApplicationWin32::SetTitle(const std::string& title) {
    m_Title = title;
    SetWindowText(m_Hwnd, m_Title.c_str());
}

void ApplicationWin32::SetResizable(bool enabled) {
    m_Resizable = enabled;

    LONG style = GetWindowLong(m_Hwnd, GWL_STYLE);

    if (enabled) {
        style |= (WS_THICKFRAME | WS_MAXIMIZEBOX);
    } else {
        style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    }

    SetWindowLong(m_Hwnd, GWL_STYLE, style);
    SetWindowPos(m_Hwnd, nullptr, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
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
        GetClientRect(m_Hwnd, &rect);
        POINT ul = { rect.left, rect.top };
        POINT lr = { rect.right, rect.bottom };
        ClientToScreen(m_Hwnd, &ul);
        ClientToScreen(m_Hwnd, &lr);
        rect = { ul.x, ul.y, lr.x, lr.y };

        ClipCursor(&rect);
    }
}

LRESULT CALLBACK ApplicationWin32::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    ApplicationWin32* app = g_App;

    switch (msg)
    {
    case WM_CLOSE:
        app->m_ShouldQuit = true;
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_SIZE:
        uint32_t w = LOWORD(lParam);
        uint32_t h = HIWORD(lParam);
        app->m_Width = w;
        app->m_Height = h;

        if (app->m_ResizeCallback)
            app->m_ResizeCallback({ w, h });
        return 0;

    case WM_KEYDOWN:
    case WM_KEYUP:
        bool pressed = (msg == WM_KEYDOWN);
        if (app->m_KeyCallback)
            app->m_KeyCallback({ (unsigned long)wParam, pressed });
        return 0;

    case WM_MOUSEMOVE:
        float x = (float)GET_X_LPARAM(lParam);
        float y = (float)GET_Y_LPARAM(lParam);

        if (app->m_MouseMoveCallback)
            app->m_MouseMoveCallback({ x, y });
        return 0;

    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
        bool pressed = (msg == WM_LBUTTONDOWN);
        if (app->m_MouseButtonCallback)
            app->m_MouseButtonCallback({ 0, pressed });
        return 0;

    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
        bool pressed = (msg == WM_RBUTTONDOWN);
        if (app->m_MouseButtonCallback)
            app->m_MouseButtonCallback({ 1, pressed });
        return 0;

    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
        bool pressed = (msg == WM_MBUTTONDOWN);
        if (app->m_MouseButtonCallback)
            app->m_MouseButtonCallback({ 2, pressed });
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

}
#endif