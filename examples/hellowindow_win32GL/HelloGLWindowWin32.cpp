// Application Context
#include "axle/core/AX_GameLoop.hpp"

#include "axle/core/app/AX_IApplication.hpp"
#include "axle/core/app/AX_ApplicationWin32.hpp"

// Render Context
#include "axle/core/ctx/AX_IRenderContext.hpp"
#include "axle/core/ctx/GL/AX_GLRenderContextWin32.hpp"
#include "axle/graphics/cmd/GL/AX_GSGLViewCommands.hpp"

// ALSoft Audio
#ifdef __AX_AUDIO_ALSOFT__
#include "axle/audio/data/AX_AudioWAV.hpp"

#include "axle/audio/AL/AX_ALAudioContext.hpp"

#include "axle/audio/AL/buffer/AX_ALAudioBufferPlayer.hpp"
#include "axle/audio/AL/buffer/AX_ALAudioBuffer.hpp"

#include "axle/audio/AL/stream/AX_ALAudioStreamVorbisPlayer.hpp"
#endif

// Graphics
#include "axle/graphics/cmd/AX_GraphicsCommand.hpp"

#include <iostream>
#include <chrono>
#include <cmath>
#include <memory>
#include <ostream>
#include <thread>

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

/*
    Official Calls Based off:
    - GL Core v330
    - GL ES 3.0 Shaders 
    + Optional Compute Shaders for further realistic effects like Dynamic HDR Exposure, etc.
    - (D3D12 / Vulkan) Code Structure
    TODO:
    - [ ] Make AudioStream Survive Tough conditions, context access locks & etc. (AKA not running out of queue easily or loops in last queued audio atleast!)
    - [ ] Gameloop freezes on Stop() at certain conditions (Unknown). added cout for further investigation, deadlock probably.
    - [ ] GS Commands for Vertex/Index/Stencil-Depth-Attachment/Texture/Texture-Sampler Buffers.
    - [ ] SLang Shading language Implementation -> Replace Variables -> Compile To SPIR-V -> GLSL/HLSL/VK
    - [ ] Scene3D Implementation with Classic Forward Renderer then Clustered-Forward Renderer
    - [ ] Vulkan/DX11 GS Command Implementations.
    - [ ] Add IRenderContext#HasSupport(An enum or some key/value definition) for verifying if host supports certain graphics properties specific for some tasks.
        Tip: We could also add CertainMethod#Requirements() or Extensions, etc. any name, just to identify which method requires which extension.
*/

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
    using namespace graphics;

    SharedPtr<core::RenderThreadContext> glThread = std::make_shared<core::RenderThreadContext>();
    SharedPtr<core::ApplicationThreadContext> appThread = std::make_shared<core::ApplicationThreadContext>();

    appThread->EnqueueTask([&appThread, &glThread]() {
        glThread->StartGraphics([&appThread]() -> std::shared_ptr<core::IRenderContext> {
            auto* ctx = new core::GLRenderContextWin32();
            ctx->Init(appThread->GetContext().get());
            if (!((core::GLRenderContextWin32*) ctx)->LoadGLFunctions()) {
                std::cerr << "Couldn't Load GLAD Functions\n";
                std::exit(1);
            }
            ctx->SetVSync(false);
            return std::shared_ptr<core::GLRenderContextWin32>(ctx);
        });
    });
    appThread->StartApp([&glThread]() -> std::shared_ptr<core::IApplication> {
        core::ApplicationSpecification spec {
            .title = "Hello Window",
            .width = HW_WIDTH,
            .height = HW_HEIGHT,
            .resizable = true
        };
        auto* app = new core::ApplicationWin32(spec);
        app->Launch();
        return std::shared_ptr<core::ApplicationWin32>(app);
    });

    appThread->AwaitStart();
    glThread->AwaitStart();

    auto appCtx = appThread->GetContext();
    auto& appState = appCtx->GetSharedState();

#ifdef __AX_AUDIO_ALSOFT__
    try {
        music.Play(&stream);
    } catch (const std::exception& ex) {
        std::cerr << "Audio Exception: " << ex.what() << std::endl;
    }
#endif

    std::atomic<float> tT{0.0f};
    glThread->PushWork("Rainbow", [&tT, glThread](){
        tT.store(tT.load() + HW_DeltaTime());
        float r, g, b;
        HW_RGBScroll(tT.load(), r, g, b);
        GS_GLCmdSetColor(r, g, b, 1.0f).Dispatch(glThread);
        GS_GLCmdClear(GSB_Color | GSB_Depth).Dispatch(glThread);
    });

    while (!appState.IsQuitting()) {
        auto eq = appState.TakeEvents();
        for (auto& e : eq) {
            if (e.type == core::EventType::WindowResize) {
                auto& ev = e.value.windowResize;
                GS_PushTNR(glThread, GS_GLCmdSetView(0, 0, ev.width, ev.height));
                break;
            }
        }
#ifdef __AX_AUDIO_ALSOFT__
        music.Tick(1);
#endif
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "1\n";
    glThread->Stop();
    std::cout << "2\n";
    glThread->GetContext()->Shutdown();
    std::cout << "3\n";

    appThread->Stop();
    std::cout << "4\n";
    appThread->GetContext()->Shutdown();
    std::cout << "5\n";

    return 0;
#else
    std::cerr << "Host is either not Win32 or doesn't support GL 330!\n";
    return -1;
#endif
}