// X11 Window Application
#include "axle/audio/AL/stream/AX_ALAudioStreamVorbis.hpp"
#include "axle/audio/AL/stream/AX_ALAudioStreamVorbisSource.hpp"
#include "axle/audio/AL/stream/AX_ALIAudioStream.hpp"
#include "axle/audio/data/AX_AudioOGG.hpp"
#include "axle/core/app/AX_ApplicationX11.hpp"
#include "axle/core/app/AX_IApplication.hpp"

// GL RenderContext For Xorg
#include "axle/core/ctx/GL/AX_GLRenderContextX11.hpp"

// GL Functions
#ifdef __AX_GRAPHICS_GL__
#include "glad/glad.h"
#endif

// ALSoft Audio
#ifdef __AX_AUDIO_ALSOFT__
#include "axle/audio/data/AX_AudioWAV.hpp"

#include "axle/audio/AL/AX_ALAudioContext.hpp"
#include "axle/audio/AL/AX_ALAudioListener.hpp"

#include "axle/audio/AL/buffer/AX_ALAudioBufferPlayer.hpp"
#include "axle/audio/AL/buffer/AX_ALAudioBuffer.hpp"

#include "axle/audio/AL/stream/AX_ALAudioStreamVorbisPlayer.hpp"
#endif

// C++ Standards
#include <exception>
#include <iostream>
#include <ostream>
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
    using clock = std::chrono::steady_clock;

    static auto last = clock::now();
    auto now = clock::now();

    std::chrono::duration<float> diff = now - last;
    last = now;

    return diff.count();
}

int main() {
#ifdef __AX_PLATFORM_X11__
    core::ApplicationSpecification spec;
    spec.title = "Hello GL X11";
    spec.width = HW_WIDTH;
    spec.height = HW_HEIGHT;
    spec.resizable = false;

    bool focused = false;
    core::ApplicationX11 app(spec);
    app.SetFocusCallback([&](const core::EventWindowFocus& f) {
        focused = f.focused;
    });
    app.SetKeyCallback([](const core::EventKey& k) {
        printf("Key %lu %s\n", k.key, k.pressed ? "pressed" : "released");
    });
    app.Launch();

#ifdef __AX_GRAPHICS_GL__
    core::GLRenderContextX11 ctx;
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

#ifdef __AX_AUDIO_ALSOFT__
    if (!audio::AL_CreateContext(nullptr)) {
        std::cerr << "Failed to create Audio Context\n";
    }

    alDistanceModel(AL_NONE);
    audio::ALAudioStreamVorbisPlayer music(4);
    music.ApplyToSources([](audio::ALAudioStreamVorbisSource& src){
        src.BypassHRTF();
    });

    audio::ALAudioBufferPlayer soundCues(16);
    audio::ALAudioBuffer buff;

    audio::AudioStreamDesc desc;
    auto ogg = audio::OGG_LoadFile("test.ogg");
    audio::ALAudioStreamVorbis stream(ogg);
    try {
        music.Play(&stream);
    } catch (const std::exception& ex) {
        std::cerr << "Audio Exception: " << ex.what() << std::endl;
    }
    try {
        auto wav = audio::WAV_LoadFile("test_mono.wav");
        if (!buff.Load(wav)) {
            std::cerr << "Failed to load test_mono.wav audio onto ALAudioBuffer\n";
        }
        //soundCues.Play(buff);
    } catch (const std::exception& ex) {
        std::cerr << "Audio Exception: " << ex.what() << std::endl;
    }
#endif

    int frames = 0;
    float t = 0.0f;
    while (!app.ShouldQuit()) {
        glViewport(0, 0, app.GetHeight(), app.GetWidth());

        float dt = HW_DeltaTime();
        t += dt;

        app.PollEvents();

        float r, g, b;
        HW_RGBScroll(t, r, g, b);
#ifdef __AX_GRAPHICS_GL__
        glClearColor(r, g, b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ctx.SwapBuffers();
#ifdef __AX_AUDIO_ALSOFT__
        float x = std::sin(1.65f * t) * 3.0f;
        float z = std::cos(1.65f * t) * 3.0f;
        audio::ALAudioListener::SetPosition(x, 0.0f, z);
        if (frames++ >= 10) {
            music.Tick(10 * dt);
        }
#endif
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