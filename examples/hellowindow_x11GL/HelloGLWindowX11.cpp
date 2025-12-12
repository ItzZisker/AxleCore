
#include "axle/core/app/AX_ApplicationX11.hpp"
#include "axle/core/app/AX_IApplication.hpp"

#include "axle/core/ctx/GL/AX_GLRenderContextX11.hpp"

#ifdef __AX_GRAPHICS_GL__
#include "glad/glad.h"
#endif

#include <chrono>
#include <cmath>

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
#ifdef __AX_PLATFORM_X11__
    axle::core::ApplicationSpecification spec;
    spec.title = "Hello GL X11";
    spec.width = HW_WIDTH;
    spec.height = HW_HEIGHT;
    spec.resizable = true;

    axle::core::ApplicationX11 app(spec);
    app.SetResizeCallback([](const axle::core::EventWindowResize& e) {
        glViewport(0, 0, (int)e.width, (int)e.height);
    });
    app.SetKeyCallback([](const axle::core::EventKey& k) {
        printf("Key %lu %s\n", k.key, k.pressed ? "pressed" : "released");
    });
    app.Launch();

#ifdef __AX_GRAPHICS_GL__
    axle::core::GLRenderContextX11 ctx;
    ctx.Init(&app);
    ctx.MakeCurrent();
    if (!ctx.LoadGLFunctions()) {
        std::cerr << "Failed to initialize GL Functions\n";
        return 1;
    }
    ctx.SetVSync(true);

    glViewport(0, 0, HW_WIDTH, HW_HEIGHT);
    glEnable(GL_DEPTH_TEST);
#endif

    float t = 0.0f;
    while (!app.ShouldQuit()) {
        app.PollEvents();
        t += HW_DeltaTime();
        float r, g, b;
        HW_RGBScroll(t, r, g, b);
#ifdef __AX_GRAPHICS_GL__
        glClearColor(r, g, b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ctx.SwapBuffers();
#endif
    }

#ifdef __AX_GRAPHICS_GL__
    ctx.Shutdown();
#endif
    app.Shutdown();
    return 0;
#else
    std::cerr << "Host doesn't support Xlib!\n";
    return -1;
#endif
}