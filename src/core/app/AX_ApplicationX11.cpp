#include "axle/core/app/AX_ApplicationX11.hpp"

#ifdef __AX_PLATFORM_X11__

#include <X11/Xlib.h>
#include <X11/Xutil.h>

namespace axle::core {

static Atom InternAtomOrZero(Display* dpy, const char* name) {
    return dpy ? XInternAtom(dpy, name, False) : 0;
}

ApplicationX11::ApplicationX11(const ApplicationSpecification& spec) : m_Spec(spec) {}
ApplicationX11::~ApplicationX11() { Shutdown(); }

void ApplicationX11::Launch() {
    if (m_Display) return; // already inited

    m_Display = XOpenDisplay(nullptr);
    if (!m_Display) {
        std::cerr << "AX Error: X11 Failed to XOpenDisplay (DISPLAY="
                  << (getenv("DISPLAY") ? getenv("DISPLAY") : "not set")
                  << ")\n";
        m_ShouldQuit = true;
        return;
    }
    m_Screen = DefaultScreen(m_Display);

    // create cursors
    m_NormalCursor = XCreateFontCursor(m_Display, XC_left_ptr);

    // hidden cursor: create an empty pixmap cursor
    {
        Pixmap blank;
        XColor dummy;
        static char data[1] = {0};
        Pixmap pixmap = XCreateBitmapFromData(m_Display, DefaultRootWindow(m_Display), data, 1, 1);
        m_HiddenCursor = XCreatePixmapCursor(m_Display, pixmap, pixmap, &dummy, &dummy, 0, 0);
        XFreePixmap(m_Display, pixmap);
    }

    CreateWindow();
}

void ApplicationX11::Shutdown() {
    DestroyWindow();

    if (m_HiddenCursor) {
        XFreeCursor(m_Display, m_HiddenCursor);
        m_HiddenCursor = 0;
    }
    if (m_NormalCursor) {
        XFreeCursor(m_Display, m_NormalCursor);
        m_NormalCursor = 0;
    }
    if (m_Display) {
        XCloseDisplay(m_Display);
        m_Display = nullptr;
    }
}

void ApplicationX11::CreateWindow(void *vi_raw) {
    if (!m_Display) return;

    int screen = m_Screen;
    Window root = RootWindow(m_Display, screen);

    XSetWindowAttributes swa;
    swa.event_mask =
        ExposureMask |
        KeyPressMask | KeyReleaseMask |
        PointerMotionMask | ButtonPressMask | ButtonReleaseMask |
        StructureNotifyMask | FocusChangeMask |
        EnterWindowMask | LeaveWindowMask;


    int depth;
    Visual* visual;
    unsigned long valuemask = CWEventMask;
    XVisualInfo* vi = (XVisualInfo*)(vi_raw);
    
    std::cout << "vi=" << vi << std::endl;

    if (vi) {
        depth  = vi->depth;
        visual = vi->visual;

        swa.colormap = XCreateColormap(m_Display, root, visual, AllocNone);
        valuemask |= CWColormap;
    } else {
        depth  = DefaultDepth(m_Display, screen);
        visual = DefaultVisual(m_Display, screen);
    }

    // set initial geometry
    m_Window = XCreateWindow(
        m_Display,
        root,
        0, 0,
        m_Spec.width,
        m_Spec.height,
        0,
        depth,
        InputOutput,
        visual,
        valuemask,
        &swa
    );

    SetTitle(m_Spec.title);

    // Polite close
    m_WMDeleteWindow = InternAtomOrZero(m_Display, "WM_DELETE_WINDOW");
    if (m_WMDeleteWindow != None) XSetWMProtocols(m_Display, m_Window, &m_WMDeleteWindow, 1);

    if (!m_Spec.resizable) {
        XSizeHints hints{};
        hints.flags = PMinSize | PMaxSize;
        hints.min_width = hints.max_width = static_cast<int>(m_Spec.width);
        hints.min_height = hints.max_height = static_cast<int>(m_Spec.height);
        XSetWMNormalHints(m_Display, m_Window, &hints);
    }

    XMapWindow(m_Display, m_Window);
    XFlush(m_Display);
}

void ApplicationX11::DestroyWindow() {
    if (!m_Display) return;
    if (m_Window) {
        XDestroyWindow(m_Display, m_Window);
        m_Window = 0;
    }
}

void ApplicationX11::PollEvents() {
    if (!m_Display) return;

    while (XPending(m_Display)) {
        XEvent ev;
        XNextEvent(m_Display, &ev);

        // Handle close
        if (ev.type == ClientMessage) {
            if (static_cast<Atom>(ev.xclient.data.l[0]) == m_WMDeleteWindow) {
                m_ShouldQuit = true;
                continue;
            }
        }

        // Window size/Configure events
        if (ev.type == ConfigureNotify) {
            XConfigureEvent const& ce = ev.xconfigure;
            // only fire if size changed
            if ((uint32_t)ce.width != m_Spec.width || (uint32_t)ce.height != m_Spec.height) {
                m_Spec.width = static_cast<uint32_t>(ce.width);
                m_Spec.height = static_cast<uint32_t>(ce.height);
                if (m_ResizeCallback) {
                    m_ResizeCallback({m_Spec.width, m_Spec.height});
                }
            }
        }

        static bool exposedEarly = false;
        // First expose: paint something so window appears (Required for wayland-to-X protocol)
        if (ev.type == Expose && !exposedEarly) {
            exposedEarly = true;
            GC gc = XCreateGC(m_Display, m_Window, 0, nullptr);
            XSetForeground(m_Display, gc, WhitePixel(m_Display, m_Screen));
            XFillRectangle(m_Display, m_Window, gc, 0, 0, m_Spec.width, m_Spec.height);
            XFreeGC(m_Display, gc);
        }

        // Keyboard / mouse events
        TranslateAndDispatchKeyEvent(ev);
        TranslateAndDispatchMouseEvent(ev);
    }

    XFlush(m_Display);
}

void ApplicationX11::SetTitle(const std::string& title) {
    m_Spec.title = title;
    if (m_Display && m_Window) {
        XStoreName(m_Display, m_Window, title.c_str());
        XFlush(m_Display);
    }
}

void ApplicationX11::SetResizable(bool enabled) {
    m_Spec.resizable = enabled;
    if (!m_Display || !m_Window) return;

    if (!enabled) {
        XSizeHints hints{};
        hints.flags = PMinSize | PMaxSize;
        hints.min_width = hints.max_width = static_cast<int>(m_Spec.width);
        hints.min_height = hints.max_height = static_cast<int>(m_Spec.height);
        XSetWMNormalHints(m_Display, m_Window, &hints);
    } else {
        // remove min/max constraints by sending empty hints
        XSizeHints hints{};
        hints.flags = 0;
        XSetWMNormalHints(m_Display, m_Window, &hints);
    }
}

void ApplicationX11::SetCursorMode(CursorMode mode) {
    m_CursorMode = mode;
    if (!m_Display || !m_Window) return;

    switch (mode) {
        case CursorMode::Normal:
            XUndefineCursor(m_Display, m_Window);
            XFlush(m_Display);
            break;
        case CursorMode::Hidden:
            if (m_HiddenCursor) {
                XDefineCursor(m_Display, m_Window, m_HiddenCursor);
                XFlush(m_Display);
            }
            break;
        case CursorMode::Locked:
            // "Locked" on X11 is typically implemented with pointer grab and warping to center.
            // attempt a pointer grab (non-blocking) and hide cursor.
            if (XGrabPointer(m_Display, m_Window, True,
                             ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                             GrabModeAsync, GrabModeAsync, m_Window, None, CurrentTime) == GrabSuccess) {
                if (m_HiddenCursor) {
                    XDefineCursor(m_Display, m_Window, m_HiddenCursor);
                }
                XFlush(m_Display);
            } else {
                std::cerr << "AX Error: X11 pointer grab failed (Lock cursor)\n";
            }
            break;
    }
}

void ApplicationX11::SetResizeCallback(std::function<void(const EventWindowResize&)> func) {
    m_ResizeCallback = std::move(func);
}

void ApplicationX11::SetKeyCallback(std::function<void(const EventKey&)> func) {
    m_KeyCallback = std::move(func);
}

void ApplicationX11::SetMouseMoveCallback(std::function<void(const EventMouseMove&)> func) {
    m_MouseMoveCallback = std::move(func);
}

void ApplicationX11::SetMouseButtonCallback(std::function<void(const EventMouseButton&)> func) {
    m_MouseButtonCallback = std::move(func);
}

// Internal helpers
void ApplicationX11::TranslateAndDispatchKeyEvent(XEvent const& xev) {
    if (!m_KeyCallback) return;

    if (xev.type == KeyPress || xev.type == KeyRelease) {
        KeySym ks;
        // translate to KeySym
        ks = XLookupKeysym(const_cast<XKeyEvent*>(&xev.xkey), 0);
        EventKey ke{};
        ke.key = ks; // engine can map KeySym -> internal codes
        ke.pressed = (xev.type == KeyPress);
        m_KeyCallback(ke);
    }
}

void ApplicationX11::TranslateAndDispatchMouseEvent(XEvent const& xev) {
    if (xev.type == MotionNotify) {
        if (m_MouseMoveCallback) {
            EventMouseMove me;
            me.x = static_cast<float>(xev.xmotion.x);
            me.y = static_cast<float>(xev.xmotion.y);
            m_MouseMoveCallback(me);
        }
    } else if (xev.type == ButtonPress || xev.type == ButtonRelease) {
        if (m_MouseButtonCallback) {
            EventMouseButton mbe;
            mbe.button = static_cast<int>(xev.xbutton.button);
            mbe.pressed = (xev.type == ButtonPress);
            m_MouseButtonCallback(mbe);
        }
    }
}

}
#endif
