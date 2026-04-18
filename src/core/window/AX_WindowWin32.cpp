#include "axle/core/window/AX_WindowWin32.hpp"
#include "axle/core/window/AX_IWindow.hpp"

#ifdef __AX_PLATFORM_WIN32__

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <windowsx.h>

#include <iostream>

#include <stdexcept>

namespace axle::core
{

static WindowWin32* g_App = nullptr; // global pointers for static WndProc (Window procedure)

WindowWin32::WindowWin32(const WindowSpec& spec, uint32_t maxSharedEvents) : IWindow(spec, maxSharedEvents) {
    if (g_App) throw std::runtime_error("Cannot have more than one application, maybe try asking microsoft to not call window events on window object construction, but just on intialization just like other opearting systems. I am lazy and not going to somehow magically overcome this");
    g_App = this;
}

WindowWin32::~WindowWin32() {
    Shutdown();
    g_App = nullptr;
}

utils::ExError WindowWin32::Launch() {
    if (m_Instance) {
        return {"Already launched Window"};
    }
    m_Instance = GetModuleHandle(nullptr);

    // Register window class
    WNDCLASS wc = {};
    wc.lpfnWndProc = (WNDPROC)WindowWin32::WndProc;
    wc.hInstance = (HINSTANCE)m_Instance;
    wc.lpszClassName = m_State.m_ClassName.c_str();
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);

    DWORD style = WS_OVERLAPPEDWINDOW;
    if (!m_State.IsResizable()) style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);

    RECT rect = { 0, 0, (LONG)m_State.GetWidth(), (LONG)m_State.GetHeight() };
    AdjustWindowRect(&rect, style, FALSE);

    m_LpClassName = wc.lpszClassName;
    m_Hwnd = CreateWindowEx(
        m_State.GetAlphaMode() != WndAlphaMode::None ? WS_EX_LAYERED : 0,
        wc.lpszClassName,
        m_State.GetTitle().c_str(),
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr,
        nullptr,
        (HINSTANCE)m_Instance,
        nullptr
    );
    if (!m_Hwnd) {
        auto errorCode = GetLastError();
        return {"Failed To Create Window: Error " + std::format("{:x}", errorCode)};
    }
    if (m_State.IsAwaitingNextEvent()) {
        m_TaskEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    }
    RAWINPUTDEVICE devices[2];

    devices[0].usUsagePage = 0x01; // Generic Desktop Controls
    devices[0].usUsage     = 0x02; // Mouse
    devices[0].dwFlags     = 0;
    devices[0].hwndTarget  = (HWND)m_Hwnd;

    devices[1].usUsagePage = 0x01; // Generic Desktop Controls
    devices[1].usUsage     = 0x06; // Keyboard
    devices[1].dwFlags     = 0;
    devices[1].hwndTarget  = (HWND)m_Hwnd;

    RegisterRawInputDevices(devices, 2, sizeof(RAWINPUTDEVICE));

    switch (m_State.GetAlphaMode()) {
        case WndAlphaMode::Constant:
            SetAlphaConstant(m_State.GetAlphaConstant());
        break;
        case WndAlphaMode::Color:
            float rgb[3];
            m_State.GetAlphaColor(rgb);
            SetAlphaColor(rgb);
        break;
    }

    ShowWindow((HWND)m_Hwnd, SW_SHOW);
    m_State.SetRunning(true);

    return utils::ExError::NoError();
}

void WindowWin32::Shutdown() {
    if (m_Hwnd) {
        DestroyWindow((HWND)m_Hwnd);
        UnregisterClass(m_LpClassName.c_str(), (HINSTANCE)m_Instance);
        m_Hwnd = nullptr;
        m_LpClassName = "";
    }
    m_State.SetRunning(false);
}

void WindowWin32::PollEvents() {
    if (m_State.m_WaitForNextEvent && m_TaskEvent) {
        MsgWaitForMultipleObjectsEx(1, &m_TaskEvent, INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
    }
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

void WindowWin32::SetAlphaConstant(float alpha) {
    if (m_State.m_AlphaMode != WndAlphaMode::Constant) return;
    BYTE _alpha = (BYTE)(alpha * 255);
    SetLayeredWindowAttributes((HWND)m_Hwnd, 0, _alpha, LWA_ALPHA);
    m_State.SetAlphaConstant(_alpha / 255.0f);
}

void WindowWin32::SetAlphaColor(float *rgb) {
    if (m_State.m_AlphaMode != WndAlphaMode::Color) return;
    COLORREF transparent = RGB((BYTE)(rgb[0] * 255), (BYTE)(rgb[1] * 255), (BYTE)(rgb[2] * 255));
    SetLayeredWindowAttributes((HWND)m_Hwnd, transparent, 0, LWA_COLORKEY);
    m_State.SetAlphaColor(rgb);
}

void WindowWin32::RequestWakeEventloop() {
    if (m_State.m_WaitForNextEvent && m_TaskEvent) {
        SetEvent(m_TaskEvent);
    }
}

void WindowWin32::SetTitle(const std::string& title) {
    SetWindowText((HWND)m_Hwnd, title.c_str());
    m_State.SetTitle(title);
}

void WindowWin32::SetResizable(bool enabled) {
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

void WindowWin32::SetCursorMode(WndCursorMode mode) {
    if (mode == WndCursorMode::Normal) {
        ShowCursor(TRUE);
        ClipCursor(nullptr);
    } else if (mode == WndCursorMode::Hidden) {
        ShowCursor(FALSE);
        ClipCursor(nullptr);
    } else if (mode == WndCursorMode::Locked) {
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

void WindowWin32::RequestQuit() {
    m_State.RequestQuit();
    RequestWakeEventloop();
}

WndKey ToWndKey(const RAWKEYBOARD& kb) {
    const bool e0 = (kb.Flags & RI_KEY_E0) != 0;
    // const bool e1 = (kb.Flags & RI_KEY_E1) != 0;

    switch (kb.VKey) {
        case VK_SHIFT:   return (kb.MakeCode == 0x36) ? WndKey::kRightShift : WndKey::kLeftShift;
        case VK_CONTROL: return e0 ? WndKey::kRightControl : WndKey::kLeftControl;
        case VK_MENU:    return e0 ? WndKey::kRightAlt     : WndKey::kLeftAlt;

        case VK_LWIN: return WndKey::kLeftCommand;
        case VK_RWIN: return WndKey::kRightCommand;
        case VK_APPS: return WndKey::kMenu;

        case VK_CAPITAL: return WndKey::kCapsLock;
        case VK_NUMLOCK: return WndKey::kNumLock;
        case VK_SCROLL:  return WndKey::kScrollLock;

        case VK_INSERT: return WndKey::kInsert;
        case VK_DELETE: return WndKey::kDelete;
        case VK_HOME:   return WndKey::kHome;
        case VK_END:    return WndKey::kEnd;
        case VK_PRIOR:  return WndKey::kPageUp;
        case VK_NEXT:   return WndKey::kPageDown;

        case VK_LEFT:  return WndKey::kArrowLeft;
        case VK_RIGHT: return WndKey::kArrowRight;
        case VK_UP:    return WndKey::kArrowUp;
        case VK_DOWN:  return WndKey::kArrowDown;

        case VK_ESCAPE: return WndKey::kEscape;
        case VK_RETURN: return e0 ? WndKey::kNumpadEnter : WndKey::kEnter;
        case VK_TAB:    return WndKey::kTab;
        case VK_BACK:   return WndKey::kBackspace;
        case VK_SPACE:  return WndKey::kSpace;

        case VK_F1:  return WndKey::kF1;
        case VK_F2:  return WndKey::kF2;
        case VK_F3:  return WndKey::kF3;
        case VK_F4:  return WndKey::kF4;
        case VK_F5:  return WndKey::kF5;
        case VK_F6:  return WndKey::kF6;
        case VK_F7:  return WndKey::kF7;
        case VK_F8:  return WndKey::kF8;
        case VK_F9:  return WndKey::kF9;
        case VK_F10: return WndKey::kF10;
        case VK_F11: return WndKey::kF11;
        case VK_F12: return WndKey::kF12;
        case VK_F13: return WndKey::kF13;
        case VK_F14: return WndKey::kF14;
        case VK_F15: return WndKey::kF15;
        case VK_F16: return WndKey::kF16;
        case VK_F17: return WndKey::kF17;
        case VK_F18: return WndKey::kF18;
        case VK_F19: return WndKey::kF19;
        case VK_F20: return WndKey::kF20;
        case VK_F21: return WndKey::kF21;
        case VK_F22: return WndKey::kF22;
        case VK_F23: return WndKey::kF23;
        case VK_F24: return WndKey::kF24;

        case VK_NUMPAD0: return WndKey::kNumpad0;
        case VK_NUMPAD1: return WndKey::kNumpad1;
        case VK_NUMPAD2: return WndKey::kNumpad2;
        case VK_NUMPAD3: return WndKey::kNumpad3;
        case VK_NUMPAD4: return WndKey::kNumpad4;
        case VK_NUMPAD5: return WndKey::kNumpad5;
        case VK_NUMPAD6: return WndKey::kNumpad6;
        case VK_NUMPAD7: return WndKey::kNumpad7;
        case VK_NUMPAD8: return WndKey::kNumpad8;
        case VK_NUMPAD9: return WndKey::kNumpad9;

        case VK_ADD:      return WndKey::kNumpadAdd;
        case VK_SUBTRACT: return WndKey::kNumpadSubtract;
        case VK_MULTIPLY: return WndKey::kNumpadMultiply;
        case VK_DIVIDE:   return WndKey::kNumpadDivide;
        case VK_DECIMAL:  return WndKey::kNumpadDecimal;

        case VK_SNAPSHOT: return WndKey::kPrintScreen;
        case VK_PAUSE:    return WndKey::kPause;

        case VK_VOLUME_MUTE: return WndKey::kVolumeMute;
        case VK_VOLUME_DOWN: return WndKey::kVolumeDown;
        case VK_VOLUME_UP:   return WndKey::kVolumeUp;

        case VK_MEDIA_NEXT_TRACK: return WndKey::kMediaNext;
        case VK_MEDIA_PREV_TRACK: return WndKey::kMediaPrev;
        case VK_MEDIA_STOP:       return WndKey::kMediaStop;
        case VK_MEDIA_PLAY_PAUSE: return WndKey::kMediaPlayPause;

        case VK_BROWSER_BACK:      return WndKey::kBrowserBack;
        case VK_BROWSER_FORWARD:   return WndKey::kBrowserForward;
        case VK_BROWSER_REFRESH:   return WndKey::kBrowserRefresh;
        case VK_BROWSER_STOP:      return WndKey::kBrowserStop;
        case VK_BROWSER_SEARCH:    return WndKey::kBrowserSearch;
        case VK_BROWSER_FAVORITES: return WndKey::kBrowserFavorites;
        case VK_BROWSER_HOME:      return WndKey::kBrowserHome;
    }

    /* ===== Alphanumerics ===== */
    if (kb.VKey >= 'A' && kb.VKey <= 'Z') {
        return static_cast<WndKey>(
            static_cast<uint32_t>(WndKey::kA) + static_cast<uint32_t>(kb.VKey - 'A')
        );
    }
    if (kb.VKey >= '0' && kb.VKey <= '9') {
        return static_cast<WndKey>(
            static_cast<uint32_t>(WndKey::k0) + static_cast<uint32_t>(kb.VKey - '0')
        );
    }

    return WndKey::kUnknown;
}

vLRESULT CALLBACK WindowWin32::WndProc(vHWND hwnd, vUINT msg, vWPARAM wParam, vLPARAM lParam) {
    WindowWin32* app = g_App;
    WndEvent event;

    switch (msg) {
        case WM_CLOSE: {
            app->m_State.RequestQuit();
            if (app->m_State.m_WaitForNextEvent) {
                CloseHandle(app->m_TaskEvent);
                app->m_TaskEvent = nullptr;
            }
            break;
        }
        case WM_DESTROY: {
            if (app->m_State.m_WaitForNextEvent) {
                CloseHandle(app->m_TaskEvent);
                app->m_TaskEvent = nullptr;
            }
            PostQuitMessage(0);
            break;
        }
        case WM_SIZE: {
            uint32_t w = LOWORD(lParam);
            uint32_t h = HIWORD(lParam);
            app->m_State.SetSize(w, h);

            event.type = WndEventType::WindowResize;
            event.value.windowResize = { w, h };
            break;
        }
        case WM_ACTIVATE: {
            app->m_Focused = (LOWORD(wParam) == WA_ACTIVE);
            event.type = WndEventType::WindowFocus;
            event.value.windowFocus = { app->m_Focused };
            break;
        }
        case WM_NCHITTEST: {
            LRESULT hit = DefWindowProc((HWND)hwnd, msg, wParam, lParam);

            if (hit == HTBOTTOM     || hit == HTTOP ||
                hit == HTRIGHT      || hit == HTLEFT ||
                hit == HTBOTTOMLEFT || hit == HTBOTTOMRIGHT ||
                hit == HTTOPLEFT    || hit == HTTOPRIGHT
            ) {
                app->m_MouseOnEdge = true;
            } else {
                app->m_MouseOnEdge = false;
            }
            return hit;
        }
        case WM_ENTERSIZEMOVE:
        case WM_EXITSIZEMOVE: {
            app->m_Grabbed = (msg == WM_ENTERSIZEMOVE);
            if (!app->m_Grabbed) app->m_MouseOnEdge = false;
            break;
        }
        case WM_INPUT: {
            UINT size = 0;
            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER));

            BYTE buffer[256];
            if (size > sizeof(buffer)) break;
            if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buffer, &size, sizeof(RAWINPUTHEADER)) != size) break;

            RAWINPUT* raw = (RAWINPUT*)buffer;
            if (raw->header.dwType == RIM_TYPEMOUSE) {
                LONG dx = raw->data.mouse.lLastX;
                LONG dy = raw->data.mouse.lLastY;

                USHORT flags = raw->data.mouse.usButtonFlags;

                app->m_State.AddMouseDX(static_cast<float>(dx));
                app->m_State.AddMouseDY(static_cast<float>(dy));

                if (flags & RI_MOUSE_LEFT_BUTTON_DOWN) {
                    app->m_State.SetMouseButtonState(WndMB::Left, true);
                } else if (flags & RI_MOUSE_LEFT_BUTTON_UP) {
                    app->m_State.SetMouseButtonState(WndMB::Left, false);
                }

                if (flags & RI_MOUSE_RIGHT_BUTTON_DOWN) {
                    app->m_State.SetMouseButtonState(WndMB::Right, true);
                } else if (flags & RI_MOUSE_RIGHT_BUTTON_UP) {
                    app->m_State.SetMouseButtonState(WndMB::Right, false);
                }

                if (flags & RI_MOUSE_MIDDLE_BUTTON_DOWN) {
                    app->m_State.SetMouseButtonState(WndMB::Middle, true);
                } else if (flags & RI_MOUSE_MIDDLE_BUTTON_UP) {
                    app->m_State.SetMouseButtonState(WndMB::Middle, false);
                }

                if (flags & RI_MOUSE_BUTTON_4_DOWN) {
                    app->m_State.SetMouseButtonState(WndMB::Misc4, true);
                } else if (flags & RI_MOUSE_BUTTON_4_UP) {
                    app->m_State.SetMouseButtonState(WndMB::Misc4, false);
                }

                if (flags & RI_MOUSE_BUTTON_5_DOWN) {
                    app->m_State.SetMouseButtonState(WndMB::Misc5, true);
                } else if (flags & RI_MOUSE_BUTTON_5_UP) {
                    app->m_State.SetMouseButtonState(WndMB::Misc5, false);
                }

                if (flags & RI_MOUSE_WHEEL) {
                    float wheelDeltaNorm = static_cast<float>(raw->data.mouse.usButtonData) / (WHEEL_DELTA);
                    app->m_State.AddMouseDWheel(wheelDeltaNorm);
                }
            } else if (raw->header.dwType == RIM_TYPEKEYBOARD) {
                const RAWKEYBOARD& kb = raw->data.keyboard;
                app->m_State.SetKeyState(ToWndKey(kb), !(kb.Flags & RI_KEY_BREAK));
            }
            return 0;
        }
    }

    if (event.type != WndEventType::Void) {
        app->m_State.PushEvent(event);
        return 0;
    }

    return DefWindowProc((HWND)hwnd, msg, wParam, lParam);
}

}
#endif