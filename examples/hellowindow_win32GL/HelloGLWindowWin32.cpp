#include "axle/core/AX_GameLoop.hpp"

#include "axle/core/app/AX_IApplication.hpp"
#include "axle/core/app/AX_ApplicationWin32.hpp"

#include "axle/core/ctx/AX_IRenderContext.hpp"
#include "axle/core/ctx/GL/AX_GLRenderContextWin32.hpp"
#include <ostream>
#include <thread>

// ALSoft Audio
#ifdef __AX_AUDIO_ALSOFT__
#include "axle/audio/data/AX_AudioWAV.hpp"

#include "axle/audio/AL/AX_ALAudioContext.hpp"
#include "axle/audio/AL/AX_ALAudioListener.hpp"

#include "axle/audio/AL/buffer/AX_ALAudioBufferPlayer.hpp"
#include "axle/audio/AL/buffer/AX_ALAudioBuffer.hpp"

#include "axle/audio/AL/stream/AX_ALAudioStreamVorbisPlayer.hpp"
#endif

#include <iostream>
#include <chrono>
#include <cmath>
#include <memory>

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
    namespace sch = std::chrono;
    
    static auto last = sch::high_resolution_clock::now();
    auto now = sch::high_resolution_clock::now();
    sch::duration<float> diff = now - last;
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
        auto wav = audio::WAV_LoadFile("test_mono.wav");
        if (!buff.Load(wav)) {
            std::cerr << "Failed to load test_mono.wav audio onto ALAudioBuffer\n";
        }
        //soundCues.Play(buff);
    } catch (const std::exception& ex) {
        std::cerr << "Audio Exception: " << ex.what() << std::endl;
    }
#endif

    core::RenderThreadContext glThread;
    core::ApplicationThreadContext appThread;

    appThread.EnqueueTask([&appThread, &glThread]() {
        glThread.StartGraphics([&appThread]() -> std::unique_ptr<core::IRenderContext> {
            auto* ctx = new core::GLRenderContextWin32();
            ctx->Init(appThread.GetContext());
            if (!((core::GLRenderContextWin32*) ctx)->LoadGLFunctions()) {
                std::cerr << "Couldn't Load GLAD Functions\n";
                std::exit(1);
            }
            ctx->SetVSync(false);
            return std::unique_ptr<core::GLRenderContextWin32>(ctx);
        });
    });
    appThread.StartApp([&glThread]() -> std::unique_ptr<core::IApplication> {
        core::ApplicationSpecification spec {
            .title = "Hello Window",
            .width = HW_WIDTH,
            .height = HW_HEIGHT,
            .resizable = true
        };
        auto* app = new core::ApplicationWin32(spec);
        app->Launch();
        return std::unique_ptr<core::ApplicationWin32>(app);
    });
    
    std::atomic<float> tT{0.0f};
    glThread.PushWork("Rainbow", [&tT](){
        tT.store(tT.load() + HW_DeltaTime());
        float r, g, b;
        HW_RGBScroll(tT.load(), r, g, b);
        glClearColor(r, g, b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    });

    appThread.AwaitStart();
    glThread.AwaitStart();

#ifdef __AX_AUDIO_ALSOFT__
    try {
        music.Play(&stream);
    } catch (const std::exception& ex) {
        std::cerr << "Audio Exception: " << ex.what() << std::endl;
    }
#endif

    auto* appCtx = appThread.GetContext();
    auto& appState = appCtx->GetSharedState();

    while (!appState.IsQuitting()) {
        // auto eq = appState.TakeEvents();
        // for (auto& e : eq) {
        //     if (e.type == core::EventType::WindowResize) {
        //         auto& ev = e.value.windowResize;
        //         // glThread.EnqueueTask([&]() {
        //         //     std::cout << "Resized to width=" << ev.width << ", height=" << ev.height << std::endl;
        //         //     glViewport(0, 0, ev.width, ev.height);
        //         // });
        //         break;
        //     }
        // }
#ifdef __AX_AUDIO_ALSOFT__
        music.Tick(1);
#endif
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    glThread.Stop();
    glThread.GetContext()->Shutdown();

    appThread.Stop();
    appThread.GetContext()->Shutdown();

    return 0;
#else
    std::cerr << "Host is either not Win32 or doesn't support GL 330!\n";
    return -1;
#endif
}