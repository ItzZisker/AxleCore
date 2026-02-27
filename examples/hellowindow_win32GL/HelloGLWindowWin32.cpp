// Application Context
#include "axle/core/window/AX_IWindow.hpp"

#include "axle/core/app/AX_Application.hpp"

// Render Context
#include "axle/core/ctx/AX_IRenderContext.hpp"
#include "axle/utils/AX_Types.hpp"

// ALSoft Audio
#ifdef __AX_AUDIO_ALSOFT__
#include "axle/audio/data/AX_AudioWAV.hpp"

#include "axle/audio/AL/AX_ALAudioContext.hpp"

#include "axle/audio/AL/buffer/AX_ALAudioBufferPlayer.hpp"
#include "axle/audio/AL/buffer/AX_ALAudioBuffer.hpp"

#include "axle/audio/AL/stream/AX_ALAudioStreamVorbisPlayer.hpp"
#endif

// Graphics
#include "axle/graphics/AX_Graphics.hpp"

#include <iostream>
#include <chrono>
#include <cmath>
#include <ostream>

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

struct MiscData {
    audio::ALAudioStreamVorbisPlayer& music;
    audio::ALAudioStreamVorbis& musicStream;

    SharedPtr<core::IWindow> ctx{nullptr};
    SharedPtr<gfx::Graphics> gfx{nullptr};
};

void InitMain(core::Application& app, void* miscData) {
    auto misc = (MiscData*) miscData;

#ifdef __AX_AUDIO_ALSOFT__
    try {
        misc->music.Play(&misc->musicStream);
    } catch (const std::exception& ex) {
        std::cerr << "Audio Exception: " << ex.what() << std::endl;
    }
#endif
    auto gfxThread = app.GetGraphicsThread();

    gfxThread->SetAutoPresent(false);
    gfxThread->CapFrames(1000.0f);

    if (!misc->ctx) {
        misc->ctx = app.GetWindowThread()->GetContext();
    }
    if (!misc->gfx) {
        misc->gfx = std::make_shared<gfx::Graphics>(gfxThread);
    }
}

void UpdateMain(float dT, core::Application& app, void* miscData) {
    auto misc = (MiscData*) miscData;

    auto ctx = misc->ctx;
    auto gfx = misc->gfx;

    auto eq = ctx->GetSharedState().TakeEvents();
    for (auto& e : eq) {
        if (e.type == core::WndEventType::WindowResize) {
            auto& ev = e.value.windowResize;
            auto cmdList = gfx->BeginCommandList();
            cmdList->Begin();
            cmdList->SetViewport(0.0f, 0.0f, float(ev.width), float(ev.height));
            //cmdList->BeginRenderPass() // How to draw clearcolor in the viewport???
            cmdList->End();
            gfx->Submit(cmdList).get();
            gfx.???
            break;
        }
    }
    
#ifdef __AX_AUDIO_ALSOFT__
    try {
        misc->music.Tick(dT);
    } catch (const std::exception& ex) {
        std::cerr << "Audio Exception: " << ex.what() << std::endl;
    }
#endif
}

int main() {
#if defined(__AX_PLATFORM_WIN32__) && defined(__AX_GRAPHICS_GL__)

#ifdef __AX_AUDIO_ALSOFT__
    if (!audio::alc::CreateContext(nullptr)) {
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
    using namespace axle::gfx;

    core::Application app;
    core::ApplicationSpec spec {
        .wndspec {
            .title = "Test",
            .width = 800,
            .height = 600,
            .resizable = true
        },
        .fixedTickRate = ChMillis(10),
        .enforceGfxType = true,
        .enforcedGfxType = core::GfxType::GL330,
    };

    MiscData misc {
        .music = music,
        .musicStream = stream
    };
    app.InitCurrent(spec, InitMain, UpdateMain, &misc);

    // std::atomic<float> tT{0.0f};
    // glThread->PushWork("Rainbow", [&tT, glThread](){
    //     tT.store(tT.load() + HW_DeltaTime());
    //     float r, g, b;
    //     HW_RGBScroll(tT.load(), r, g, b);
    //     GS_GLCmdSetColor(r, g, b, 1.0f).Dispatch(glThread);
    //     GS_GLCmdClear(GSB_Color | GSB_Depth).Dispatch(glThread);
    // });

    return 0;
#else
    std::cerr << "Host is either not Win32 or doesn't support GL 330!\n";
    return -1;
#endif
}