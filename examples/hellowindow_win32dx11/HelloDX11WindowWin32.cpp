
#include "axle/core/app/AX_ApplicationWin32.hpp"
#include "axle/core/app/AX_IApplication.hpp"

#include "axle/core/ctx/DX11/AX_DX11RenderContextWin32.hpp"

#include <chrono>
#include <cmath>

#include <unistd.h>

#define HW_WIDTH  800
#define HW_HEIGHT 600

using namespace axle;

// t is accumulated time (+= deltaTime)
void HW_RGBScroll(float t, float& r, float& g, float& b) {
    r = 0.5f + 0.5f * std::sin(t);
    g = 0.5f + 0.5f * std::sin(t + 2.094f);
    b = 0.5f + 0.5f * std::sin(t + 4.188f);
}

float HW_DeltaTime() {
    static auto last = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> diff = now - last;
    last = now;
    return diff.count();
}

int main() {
#ifdef __AX_PLATFORM_WIN32__
    core::ApplicationSpecification spec;
    spec.title = "Hello Win32";
    spec.width = HW_WIDTH;
    spec.height = HW_HEIGHT;
    spec.resizable = true;

    core::ApplicationWin32 app(spec);
    app.SetKeyCallback([](const core::EventKey& k) {
        printf("Key %lu %s\n", k.key, k.pressed ? "pressed" : "released");
    });
    app.Launch();

    core::DX11RenderContextWin32 ctx;
    ctx.Init(&app);
    ctx.SetVSync(false);

    float t = 0.0f;
    while (!app.ShouldQuit()) {
        app.PollEvents();
        t += HW_DeltaTime();
        float r, g, b;
        HW_RGBScroll(t, r, g, b);
        float color[4] = { r, g, b, 1.0f };
        ctx.GetDeviceContext()->ClearRenderTargetView(ctx.GetRenderTargetView().Get(), color);
        ctx.SwapBuffers();
    }

    app.Shutdown();
    return 0;
#else
    std::cerr << "Host is not Win32!\n";
    return -1;
#endif
}