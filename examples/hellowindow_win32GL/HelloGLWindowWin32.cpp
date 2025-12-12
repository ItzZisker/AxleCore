
#include "axle/core/app/AX_ApplicationWin32.hpp"
#include "axle/core/app/AX_IApplication.hpp"

#include "axle/core/ctx/AX_IRenderContext.hpp"
#include "axle/core/ctx/GL/AX_GLRenderContextWin32.hpp"

#include <chrono>
#include <cmath>

#include <iostream>
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
#if defined(__AX_PLATFORM_WIN32__) && defined(__AX_GRAPHICS_GL__)
    core::ApplicationSpecification spec;
    spec.title = "Hello Win32";
    spec.width = HW_WIDTH;
    spec.height = HW_HEIGHT;
    spec.resizable = true;

    core::ApplicationWin32 app(spec);
    app.SetKeyCallback([](const core::EventKey& k) {
        printf("Key %lu %s\n", k.key, k.pressed ? "pressed" : "released");
    });
    app.SetResizeCallback([](const core::EventWindowResize& r){
        //glViewport(0, 0, static_cast<int>(r.width), static_cast<int>(r.height));
    });
    app.Launch();

    core::GLRenderContextWin32 ctx;
    ctx.Init(&app);
    ctx.LoadGLFunctions();
    if (!ctx.LoadGLFunctions()) {
        std::cerr << "Couldn't Load GLAD Functions\n";
        return 1;
    }
    ctx.SetVSync(false);

    glViewport(0, 0, HW_WIDTH, HW_HEIGHT);
    glEnable(GL_DEPTH_TEST);

    float t = 0.0f;
    while (!app.ShouldQuit()) {
        app.PollEvents();
        t += HW_DeltaTime();
        float r, g, b;
        HW_RGBScroll(t, r, g, b);
        glClearColor(r, g, b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ctx.SwapBuffers();
    }

    app.Shutdown();
    return 0;
#else
    std::cerr << "Host is either not Win32 or doesn't support GL 330!\n";
    return -1;
#endif
}