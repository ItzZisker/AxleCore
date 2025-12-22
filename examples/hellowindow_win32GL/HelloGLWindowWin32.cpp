
#include "axle/core/app/AX_ApplicationWin32.hpp"
#include "axle/core/app/AX_IApplication.hpp"

#include "axle/core/ctx/AX_IRenderContext.hpp"
#include "axle/core/ctx/GL/AX_GLRenderContextWin32.hpp"

// ALSoft Audio
#ifdef __AX_AUDIO_ALSOFT__
#include "axle/audio/data/AX_AudioWAV.hpp"

#include "axle/audio/AL/AX_ALAudioContext.hpp"
#include "axle/audio/AL/AX_ALAudioListener.hpp"

#include "axle/audio/AL/buffer/AX_ALAudioBufferPlayer.hpp"
#include "axle/audio/AL/buffer/AX_ALAudioBuffer.hpp"

#include "axle/audio/AL/stream/AX_ALAudioStreamVorbisPlayer.hpp"
#endif

#include <chrono>
#include <cmath>
#include <thread>
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

#ifdef __AX_AUDIO_ALSOFT__
    if (!audio::AL_CreateContext(nullptr)) {
        std::cerr << "Failed to create Audio Context\n";
    }

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

    core::GLRenderContextWin32 ctx;
    ctx.Init(&app);
    ctx.LoadGLFunctions();
    if (!ctx.LoadGLFunctions()) {
        std::cerr << "Couldn't Load GLAD Functions\n";
        return 1;
    }
    ctx.SetVSync(false);

    glEnable(GL_DEPTH_TEST);

    float t = 0.0f;
    while (!app.ShouldQuit()) {
        //app.PollEvents();
        glViewport(0, 0, app.GetWidth(), app.GetHeight());
        float dt = HW_DeltaTime();
        t += dt;
        float r, g, b;
        HW_RGBScroll(t, r, g, b);
        glClearColor(r, g, b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        if (!app.IsThrottling()) {
            //ctx.SwapBuffers();
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
#ifdef __AX_AUDIO_ALSOFT__
        float x = std::sin(1.65f * t) * 3.0f;
        float z = std::cos(1.65f * t) * 3.0f;
        audio::ALAudioListener::SetPosition(x, 0.0f, z);
        music.Tick(dt);
#endif
    }

    app.Shutdown();
    return 0;
#else
    std::cerr << "Host is either not Win32 or doesn't support GL 330!\n";
    return -1;
#endif
}