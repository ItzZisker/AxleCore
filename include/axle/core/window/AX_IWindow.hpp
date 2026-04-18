#pragma once

#include <deque>
#include <mutex>
#include <string>

#include "axle/utils/AX_Expected.hpp"

// IMPORTANT!
// We have to implement joystick calliberation/choose or anything support later;
// evdev/libinput/idk for linux & DirectInput/XInput for windows, for android (yeah we're cooked) & for Mac, I have no idea (we are super-cooked).

namespace axle::gfx
{
    class IRenderContext;

    class RenderContextGLWin32;
    class RenderContextGLX11;
    class RenderContextD3D11Win32;
};

namespace axle::core
{

enum class WndKey : uint32_t {
    __Begin__,

    kUnknown,

    /* ===== Function keys ===== */
    kF1,  kF2,  kF3,  kF4,  kF5,  kF6,
    kF7,  kF8,  kF9,  kF10, kF11, kF12,
    kF13, kF14, kF15, kF16, kF17, kF18,
    kF19, kF20, kF21, kF22, kF23, kF24,

    /* ===== Number row ===== */
    k0, k1, k2, k3, k4, k5, k6, k7, k8, k9,

    /* ===== Letters ===== */
    kA, kB, kC, kD, kE, kF, kG,
    kH, kI, kJ, kK, kL, kM,
    kN, kO, kP, kQ, kR, kS,
    kT, kU, kV, kW, kX, kY, kZ,

    /* ===== Symbols ===== */
    kGrave,
    kMinus,
    kEquals,
    kLeftBracket,
    kRightBracket,
    kBackSlash,
    kSemicolon,
    kApostrophe,
    kComma,
    kPeriod,
    kSlash,

    /* ===== Whitespace & control ===== */
    kEscape,
    kEnter,
    kTab,
    kBackspace,
    kSpace,

    /* ===== Navigation ===== */
    kInsert,
    kDelete,
    kHome,
    kEnd,
    kPageUp,
    kPageDown,

    kArrowLeft,
    kArrowRight,
    kArrowUp,
    kArrowDown,

    /* ===== Modifiers ===== */
    kLeftShift,
    kRightShift,
    kLeftControl,
    kRightControl,
    kLeftAlt,
    kRightAlt,
    kLeftCommand,   // Windows key
    kRightCommand,
    kMenu,          // Application / context menu
    kCapsLock,
    kNumLock,
    kScrollLock,

    /* ===== Numpad ===== */
    kNumpad0,
    kNumpad1,
    kNumpad2,
    kNumpad3,
    kNumpad4,
    kNumpad5,
    kNumpad6,
    kNumpad7,
    kNumpad8,
    kNumpad9,

    kNumpadAdd,
    kNumpadSubtract,
    kNumpadMultiply,
    kNumpadDivide,
    kNumpadDecimal,
    kNumpadEnter,
    kNumpadEqual,

    /* ===== System / special ===== */
    kPrintScreen,
    kPause,

    /* ===== Browser / media ===== */
    kVolumeMute,
    kVolumeDown,
    kVolumeUp,
    kMediaNext,
    kMediaPrev,
    kMediaStop,
    kMediaPlayPause,

    kBrowserBack,
    kBrowserForward,
    kBrowserRefresh,
    kBrowserStop,
    kBrowserSearch,
    kBrowserFavorites,
    kBrowserHome,

    /* ===== IME / international ===== */
    kKana,
    kKanji,
    kConvert,
    kNonConvert,
    kLang1,
    kLang2,
    kLang3,
    kLang4,
    kLang5,

    /* ===== OEM ===== */
    kOEM102, // < > | on ISO keyboards

    __End__
};

enum class WndMB : uint8_t {
    __Begin__,

    Left,
    Right,
    Middle,
    Misc4,
    Misc5,

    __End__
};

enum class WndCursorMode {
    Normal,
    Hidden,
    Locked // centered & raw mouse movement
};

enum class WndAlphaMode {
    None,
    Constant,
    Color,
};

enum class WndEdgesMode {
    Popup,
    Bordered
};

struct WindowSpec {
    std::string className{"Ax App"};
    std::string title{"AX App"};

    uint32_t width{1024};
    uint32_t height{768};

    WndEdgesMode edgesMode{WndEdgesMode::Bordered};
    WndAlphaMode alphaMode{WndAlphaMode::None};
    float alphaModeConstant{1.0f};
    float alphaModeColor[3] = {0.0f, 0.0f, 0.0f};

    bool resizable{true};
    bool waitForNextEvent{true};
};

enum WindowType {
    Win32,
    X11,
    Cocoa,
    SurfaceFlinger,
    Unknown
};

enum WndEventType {
    Void,
    WindowResize,
    WindowFocus,
    Quit
};

union WndEventValue {
    struct { uint32_t width, height; } windowResize;
    struct { bool focused; } windowFocus;
};

struct WndEvent {
    WndEventType type{WndEventType::Void};
    WndEventValue value;
};

class DiscreteState {
public:
    explicit DiscreteState(WindowSpec spec = {}, uint32_t maxEventsQueue = 64);

    bool IsRunning() const;
    bool IsResizable() const;
    bool IsQuitting() const;
    bool IsAwaitingNextEvent() const;

    WndCursorMode GetCursorMode() const;
    WndAlphaMode GetAlphaMode() const;
    float GetAlphaConstant() const;
    void GetAlphaColor(float *rgbOut) const;

    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    uint32_t GetMaxEventQueueCapacity() const;

    float GetMouseX() const;
    float GetMouseY() const;

    float PeekMouseDX();
    float PeekMouseDY();
    float PeekMouseDWheel();

    bool IsKeyPressed(WndKey key) const;
    bool IsMouseButtonPressed(WndMB mb) const;

    std::string GetClassName() const;
    std::string GetTitle() const;

    std::deque<WndEvent> TakeEvents();
protected:
    void SetRunning(bool running);
    void SetResizable(bool resizable);
    void SetTitle(const std::string& title);
    void SetSize(uint32_t width, uint32_t height);
    void SetCursorMode(WndCursorMode mode);
    void SetAlphaConstant(float alpha);
    void SetAlphaColor(float *rgb);

    void SetMouseX(float mouseX);
    void SetMouseY(float mouseY);

    void AddMouseDX(float mouseDX);
    void AddMouseDY(float mouseDY);
    void AddMouseDWheel(float mouseDWheel);

    void SetMouseButtonState(WndMB mb, bool pressed);
    void SetKeyState(WndKey key, bool pressed);

    void PushEvent(const WndEvent& event);

    void RequestQuit();
private:
    friend class IWindow;

    friend class WindowWin32;
    friend class WindowX11;

    WndCursorMode m_CursorMode{WndCursorMode::Normal};
    uint32_t m_Width{800}, m_Height{600};

    uint32_t m_KeysDown[std::size_t(WndKey::__End__)];
    uint8_t  m_MouseButtonsDown[std::size_t(WndMB::__End__)];

    float m_MouseDX{0}, m_MouseDY{0}, m_MouseDWheel{0};
    float m_MouseX{0}, m_MouseY{0};

    uint32_t m_MaxEventQueueCapacity{64};
    std::deque<WndEvent> m_SharedEventsQ;

    bool m_WaitForNextEvent{false};
    bool m_IsRunning{false};
    bool m_IsResizable{false};
    bool m_ShouldQuit{false};

    std::string m_ClassName{"Pylo App"};
    std::string m_Title{"Pylo App"};

    float m_Alpha{1.0f};
    float m_AlphaColor[3] = {0.0f, 0.0f, 0.0f};
    WndAlphaMode m_AlphaMode{WndAlphaMode::None};

    mutable std::mutex m_Mutex;
};

class IWindow {
public:
    IWindow(const WindowSpec& spec, uint32_t maxSharedEvents)
        : m_State(spec, maxSharedEvents) {}

    IWindow(const IWindow&) = delete;
    IWindow& operator=(const IWindow&) = delete;

    IWindow(IWindow&&) = delete;
    IWindow& operator=(const IWindow&&) = delete;

    virtual ~IWindow() = default;
    virtual utils::ExError Launch() = 0;

    virtual void Shutdown() = 0;
    virtual void PollEvents() = 0;

    virtual void SetTitle(const std::string& title) = 0;
    virtual void SetResizable(bool enabled) = 0;
    virtual void SetCursorMode(WndCursorMode mode) = 0;
    virtual void SetAlphaConstant(float alpha) = 0;
    virtual void SetAlphaColor(float *rgb) = 0;

    virtual void RequestWakeEventloop() = 0;
    virtual void RequestQuit() { m_State.RequestQuit(); }

    virtual DiscreteState& GetDiscreteState() { return m_State; }
    virtual WindowType GetType() const = 0;

    // Backend-specific pointer access (GL/DX/VK surfaces, handles, etc)
    virtual void* GetNativeWindowHandle() = 0;
protected:
    friend class gfx::IRenderContext;

    friend class gfx::RenderContextGLWin32;
    friend class gfx::RenderContextGLX11;
    friend class gfx::RenderContextD3D11Win32;

    std::mutex m_HandleMutex;

    mutable DiscreteState m_State;
};

}
