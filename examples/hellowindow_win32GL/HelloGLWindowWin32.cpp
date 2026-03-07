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
// Graphics Scene
#include "axle/graphics/scene/AX_Camera.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#include <iostream>
#include <chrono>
#include <cmath>
#include <ostream>

#define HW_WIDTH  800
#define HW_HEIGHT 600

using namespace axle;
using namespace axle::utils;

/*
    Official Calls Based off:
    - GL Core v330
    - GL ES 3.0 Shaders 
    + Optional Compute Shaders for further realistic effects like Dynamic HDR Exposure, etc.
    - (D3D12 / Vulkan) Code Structure
    TODO:
    - [ ] Make AudioStream Survive Tough conditions, context access locks & etc. (AKA not running out of queue easily or loops in last queued audio atleast!)
    - [*] Gameloop freezes on Stop() at certain conditions (Unknown). added cout for further investigation, deadlock probably.
    - [*] GS Commands for Vertex/Index/Stencil-Depth-Attachment/Texture/Texture-Sampler Buffers.
    - [*] SLang Shading language Implementation -> Replace Variables -> Compile To SPIR-V -> GLSL/HLSL/VK
    - [ ] Scene3D Implementation with Classic Forward Renderer then Clustered-Forward Renderer
    - [ ] Vulkan/DX11 GS Command Implementations.
    - [-] Add IRenderContext#HasSupport(An enum or some key/value definition) for verifying if host supports certain graphics properties specific for some tasks.
        Tip: We could also add CertainMethod#Requirements() or Extensions, etc. any name, just to identify which method requires which extension.
*/

struct MatrixUBO {
    glm::mat4 MVP[3];

    MatrixUBO() {
        MVP[0] = glm::mat4(1.0f);
        MVP[1] = glm::lookAt(
            glm::vec3(2.5f, 2.5f, 2.5f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        MVP[2] = glm::perspective(
            glm::radians(70.0f),
            800.0f / 600.0f,
            0.1f,
            100.0f
        );
    }
    MatrixUBO(scene::Camera& camera, core::DiscreteState& state) {
        float fwidth = float(state.GetWidth());
        float fheight = float(state.GetHeight());

        if (fwidth == 0) fwidth = 1.0f;
        if (fheight == 0) fheight = 1.0f;

        MVP[0] = glm::mat4(1.0f);
        MVP[1] = camera.GetViewMatrix();
        MVP[2] = glm::perspective(glm::radians(70.0f), fwidth / fheight, 0.1f, 100.0f);
    }
};

struct UBOState {
    MatrixUBO matrices;
    bool reqUpdateUBO{true};

    MatrixUBO& Peek() {
        reqUpdateUBO = false;
        return matrices;
    }
    void Update(scene::Camera& camera, core::DiscreteState& state) {
        reqUpdateUBO = true;
        matrices = MatrixUBO(camera, state);
    }
};

void GenerateCube(std::vector<float>& vertexData, std::vector<uint32_t>& indexData) {
    const float cubeVertices[24 * 8] = {
        // Front Face (+Z, Normal: 0, 0, 1)
        -1.0f, -1.0f,  1.0f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f,
         1.0f, -1.0f,  1.0f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f,
         1.0f,  1.0f,  1.0f,   0.0f, 0.0f, 1.0f,   1.0f, 0.0f,
        -1.0f,  1.0f,  1.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,
        // Back Face (-Z, Normal: 0, 0, -1)
         1.0f, -1.0f, -1.0f,   0.0f, 0.0f, -1.0f,   0.0f, 1.0f,
        -1.0f, -1.0f, -1.0f,   0.0f, 0.0f, -1.0f,   1.0f, 1.0f,
        -1.0f,  1.0f, -1.0f,   0.0f, 0.0f, -1.0f,   1.0f, 0.0f,
         1.0f,  1.0f, -1.0f,   0.0f, 0.0f, -1.0f,   0.0f, 0.0f,
        // Right Face (+X, Normal: 1, 0, 0)
         1.0f, -1.0f,  1.0f,   1.0f, 0.0f, 0.0f,   0.0f, 1.0f,
         1.0f, -1.0f, -1.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
         1.0f,  1.0f, -1.0f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
         1.0f,  1.0f,  1.0f,   1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
        // Left Face (-X, Noral -1, 0, 0)
        -1.0f, -1.0f, -1.0f,   -1.0f, 0.0f, 0.0f,   0.0f, 1.0f,
        -1.0f, -1.0f,  1.0f,   -1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
        -1.0f,  1.0f,  1.0f,   -1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
        -1.0f,  1.0f, -1.0f,   -1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
        // Top Face (+Y, Normal: 0, 1, 0)
        -1.0f,  1.0f,  1.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
         1.0f,  1.0f,  1.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
         1.0f,  1.0f, -1.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
        -1.0f,  1.0f, -1.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
        // Bottom Face (-Y, Normal: 0, -1, 0)
        -1.0f, -1.0f, -1.0f,   0.0f, -1.0f, 0.0f,   0.0f, 1.0f,
         1.0f, -1.0f, -1.0f,   0.0f, -1.0f, 0.0f,   1.0f, 1.0f,
         1.0f, -1.0f,  1.0f,   0.0f, -1.0f, 0.0f,   1.0f, 0.0f,
        -1.0f, -1.0f,  1.0f,   0.0f, -1.0f, 0.0f,   0.0f, 0.0f 
    };
    const uint32_t cubeIndices[36] = {
        0, 1, 2, 2, 3, 0,
        5, 4, 7, 7, 6, 5, 
        8, 9, 10, 10, 11, 8,
        13, 12, 15, 15, 14, 13,
        16, 17, 18, 18, 19, 16,
        21, 20, 23, 23, 22, 21
    };
    for (size_t i{0}; i < 24 * 8; i++) vertexData.push_back(cubeVertices[i]);
    for (size_t i{0}; i < 36; i++) indexData.push_back(cubeIndices[i]);
}

struct WindowData {
    SharedPtr<core::ThreadContextWnd> wndThread;
    SharedPtr<std::atomic<float>> accTimeAtomic;
};

void UpdateWnd(WindowData& wndData) {
    auto wndctx = wndData.wndThread->GetContext();
    float accTime = wndData.accTimeAtomic->load(std::memory_order_relaxed);

    // wndctx->SetAlpha(0.75f + 0.25f * std::sin(0.65f * accTime));
}

struct RenderData {
    SharedPtr<core::ThreadContextWnd> wndThread;
    SharedPtr<core::ThreadContextGfx> gfxThread;
    SharedPtr<gfx::ICommandList> cmdList;

    gfx::RenderPassHandle defPass{};
    gfx::RenderPipelineHandle cubePipeline{};
    gfx::BufferHandle cubeVertexBuffer{};
    gfx::BufferHandle cubeIndexBuffer{};
    gfx::BufferHandle cubeUniformBuffer{};
    gfx::ResourceSetHandle cubeResourceSet{};
    uint32_t cubeIndexCount{0};

    std::atomic_bool mouseCaptured{false};
    bool escPressed{false};
    scene::Camera camera{glm::vec3(2.0f, 0.5f, 0.5f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)};
    float yaw{0.0f}, pitch{0.0f};
    float sensitivity = 0.08f;
    SharedPtr<UBOState> uboState{std::make_shared<UBOState>()};

    uint32_t lastWidth{0}, lastHeight{0};
};

void UpdateRender(RenderData& rdrData) {
    auto gfxThread = rdrData.gfxThread;
    auto gbgfx = gfxThread->GetContext();

    float dT = gfxThread->GetLastFrameTime();
    gfx::RenderPassClear clearArgs{};
    clearArgs.clearDepth = 1.0f;
    clearArgs.clearStencil = 0;
    clearArgs.clearColor[0] = clearArgs.clearColor[1] = clearArgs.clearColor[2] = 0.1f;
    clearArgs.clearColor[3] = 1.0f;
    // clearArgs.clearColor[0] = 0.5f + 0.5f * std::sin(accTime);
    // clearArgs.clearColor[1] = 0.5f + 0.5f * std::sin(accTime + 2.094f);
    // clearArgs.clearColor[2] = 0.5f + 0.5f * std::sin(accTime + 4.188f);

    auto wndThread = rdrData.wndThread;
    auto wndCtx = wndThread->GetContext();

    auto& wndState = wndCtx->GetDiscreteState();

    auto eventsQueue = wndState.TakeEvents();
    for (auto& event : eventsQueue) {
        if (event.type == core::WndEventType::WindowResize) {
            rdrData.uboState->Update(rdrData.camera, wndState);
        }
    }

    float xrel = wndState.PeekMouseDX();
    float yrel = wndState.PeekMouseDY();

    if (rdrData.mouseCaptured && (xrel != 0 || yrel != 0)) {
        xrel *= rdrData.sensitivity;
        yrel *= rdrData.sensitivity;

        rdrData.yaw += xrel;
        rdrData.pitch += yrel;

        rdrData.yaw = fmod(rdrData.yaw, 360.0f);
        rdrData.pitch = glm::clamp(rdrData.pitch, -87.0f, 87.0f);

        rdrData.camera.SetDirection(Coordination::DirectionOf(
            rdrData.yaw,
            rdrData.pitch
        ));
        rdrData.uboState->Update(rdrData.camera, wndState);
    }

    if (!wndState.IsKeyPressed(core::WndKey::kEscape) && rdrData.escPressed) {
        rdrData.escPressed = false;
    }
    if (wndState.IsKeyPressed(core::WndKey::kEscape) && !rdrData.escPressed) {
        rdrData.escPressed = true;
        wndThread->EnqueueTask([wndCtx, mouseCaptured = &rdrData.mouseCaptured]() {
            auto mc = mouseCaptured->load();

            wndCtx->SetCursorMode(mc ?
                core::WndCursorMode::Normal :
                core::WndCursorMode::Locked
            );
            mouseCaptured->store(!mc);
        });
        wndThread->SignalWindow();
    }

    const float cameraSpeed = 2.6f * dT;

    glm::vec3 horizontalDirection(0.0f);

    horizontalDirection.z = -sin(glm::radians(rdrData.yaw));
    horizontalDirection.x = -cos(glm::radians(rdrData.yaw));

    glm::vec3 cameraPos = rdrData.camera.GetPosition();
    glm::vec3 cameraUp = rdrData.camera.GetUp();

    bool queCameraUpdate{false};

    if (wndState.IsKeyPressed(core::WndKey::kW)) {
        cameraPos += cameraSpeed * horizontalDirection;
        queCameraUpdate = true;
    }
    if (wndState.IsKeyPressed(core::WndKey::kS)) {
        cameraPos -= cameraSpeed * horizontalDirection;
        queCameraUpdate = true;
    }
    if (wndState.IsKeyPressed(core::WndKey::kA)) {
        cameraPos -= glm::normalize(glm::cross(horizontalDirection, cameraUp)) * cameraSpeed;
        queCameraUpdate = true;
    }
    if (wndState.IsKeyPressed(core::WndKey::kD)) {
        cameraPos += glm::normalize(glm::cross(horizontalDirection, cameraUp)) * cameraSpeed;
        queCameraUpdate = true;
    }
    if (wndState.IsKeyPressed(core::WndKey::kSpace)) {
        cameraPos += cameraSpeed * glm::vec3(0.0f, 1.0f, 0.0f);
        queCameraUpdate = true;
    }
    if (wndState.IsKeyPressed(core::WndKey::kLeftShift)) {
        cameraPos -= cameraSpeed * glm::vec3(0.0f, 1.0f, 0.0f);
        queCameraUpdate = true;
    }

    if (queCameraUpdate) {
        rdrData.camera.SetPosition(cameraPos);
        rdrData.uboState->Update(rdrData.camera, wndState);
    }

    if (rdrData.uboState->reqUpdateUBO) {
        auto& mvp = rdrData.uboState->Peek();
        gbgfx->UpdateBuffer(rdrData.cubeUniformBuffer, 0, sizeof(MatrixUBO), &mvp).ThrowIfValid();
    }

    uint32_t imgIndex = ExpectOrThrow(gbgfx->AcquireNextImage());

    auto& appState = rdrData.wndThread->GetContext()->GetDiscreteState();
    uint32_t width = appState.GetWidth();
    uint32_t height = appState.GetHeight();

    auto cmdList = rdrData.cmdList;
    cmdList->Begin();
    if (width != rdrData.lastWidth || height != rdrData.lastHeight) {
        rdrData.lastWidth = width;
        rdrData.lastHeight = height;

        cmdList->SetViewport({0.0f, 0.0f, float(rdrData.lastWidth), float(rdrData.lastHeight)});
    }
    cmdList->BeginRenderPass({rdrData.defPass, ExpectOrThrow(gbgfx->GetSwapchainFramebuffer(imgIndex)), clearArgs});
    cmdList->BindRenderPipeline({rdrData.cubePipeline});
    cmdList->BindVertexBuffer({rdrData.cubeVertexBuffer});
    cmdList->BindIndexBuffer({rdrData.cubeIndexBuffer});
    cmdList->BindResourceSet({rdrData.cubeResourceSet});
    cmdList->DrawIndexed({rdrData.cubeIndexCount, 0});
    cmdList->EndRenderPass({rdrData.defPass});
    cmdList->End();

    gbgfx->Execute(*cmdList.get()).ThrowIfValid();
}

void InitRender(RenderData& rdrData) {
    auto gbgfx = rdrData.gfxThread->GetContext();

    gfx::DefaultRenderPassDesc drpdesc{};
    drpdesc.colorOps.load = gfx::LoadOp::Clear;
    drpdesc.colorOps.store = gfx::StoreOp::Store;
    drpdesc.depthStencilOps.load = gfx::LoadOp::Clear;
    drpdesc.depthStencilOps.store = gfx::StoreOp::Discard;

    rdrData.defPass = ExpectOrThrow(gbgfx->CreateDefaultRenderPass(drpdesc));

    auto& appState = rdrData.wndThread->GetContext()->GetDiscreteState();
    rdrData.lastWidth = appState.GetWidth();
    rdrData.lastHeight = appState.GetHeight();

    gfx::ShaderEntryPoint shaderVertexEntry;
    shaderVertexEntry.name = "mainVertex";
    shaderVertexEntry.stage = gfx::ShaderStage::Vertex;

    gfx::ShaderEntryPoint shaderFragmentEntry;
    shaderFragmentEntry.name = "mainFragment";
    shaderFragmentEntry.stage = gfx::ShaderStage::Fragment;

    gfx::ShaderDesc shaderCubeDesc;
    shaderCubeDesc.sourcePath = "cube.slang";
    shaderCubeDesc.entryPoints = {shaderVertexEntry, shaderFragmentEntry};

    auto shaderCube = ExpectOrThrow(gbgfx->CreateProgram(shaderCubeDesc));

    gfx::VertexLayout layout;
    gfx::VertexTypeDesc typeDesc {
        gfx::VertexAttributeClass::Float, 
        gfx::VertexAttributeType::Float32
    };

    uint32_t currentOffset = 0;
    const uint32_t FLOAT_SIZE = sizeof(float); // 4 bytes

    // 1. Position (float3, location 0)
    layout.attributes.push_back({0, 3, 0, currentOffset, FLOAT_SIZE * 3, typeDesc, false});
    currentOffset += FLOAT_SIZE * 3;
    // 2. Normal (float3, location 1)
    layout.attributes.push_back({1, 3, 0, currentOffset, FLOAT_SIZE * 3, typeDesc, false});
    currentOffset += FLOAT_SIZE * 3;
    // 3. TexCoord (float2, location 2)
    layout.attributes.push_back({2, 2, 0, currentOffset, FLOAT_SIZE * 2, typeDesc, false});
    currentOffset += FLOAT_SIZE * 2;

    layout.stride = currentOffset; 

    gfx::RenderPipelineDesc psoDesc{};
    psoDesc.renderPass = rdrData.defPass;
    psoDesc.shader = shaderCube;
    psoDesc.layout = layout;
    psoDesc.raster.cull = gfx::CullMode::None;
    psoDesc.blend.enabled = true;
    rdrData.cubePipeline = ExpectOrThrow(gbgfx->CreateRenderPipeline(psoDesc));

    std::vector<float> cubeVertices; std::vector<uint32_t> cubeIndices;
    GenerateCube(cubeVertices, cubeIndices);

    gfx::BufferDesc cubeVboDesc{};
    cubeVboDesc.size = cubeVertices.size() * sizeof(float);
    cubeVboDesc.usage = gfx::BufferUsage::Vertex;
    cubeVboDesc.access = gfx::BufferAccess::Immutable;
    cubeVboDesc.cpuVisible = false;
    rdrData.cubeVertexBuffer = ExpectOrThrow(gbgfx->CreateBuffer(cubeVboDesc));
    gbgfx->UpdateBuffer(rdrData.cubeVertexBuffer, 0, cubeVboDesc.size, cubeVertices.data()).ThrowIfValid();

    gfx::BufferDesc cubeEboDesc{};
    cubeEboDesc.size = cubeIndices.size() * sizeof(uint32_t);
    cubeEboDesc.usage = gfx::BufferUsage::Index;
    cubeEboDesc.access = gfx::BufferAccess::Immutable;
    cubeEboDesc.cpuVisible = false;
    rdrData.cubeIndexBuffer = ExpectOrThrow(gbgfx->CreateBuffer(cubeEboDesc));
    rdrData.cubeIndexCount = cubeIndices.size();
    gbgfx->UpdateBuffer(rdrData.cubeIndexBuffer, 0, cubeEboDesc.size, cubeIndices.data()).ThrowIfValid();

    gfx::BufferDesc cubeUboDesc{};
    cubeUboDesc.size = 3 * 4 * 4 * sizeof(float);
    cubeUboDesc.usage = gfx::BufferUsage::Uniform;
    cubeUboDesc.access = gfx::BufferAccess::Dynamic;
    cubeEboDesc.cpuVisible = true;
    rdrData.cubeUniformBuffer = ExpectOrThrow(gbgfx->CreateBuffer(cubeUboDesc));

    gfx::Binding cubeUboBinding{};
    cubeUboBinding.type = gfx::BindingType::UniformBuffer;
    cubeUboBinding.resource = gfx::ResourceHandle(rdrData.cubeUniformBuffer);
    cubeUboBinding.range = cubeUboDesc.size;

    gfx::ResourceSetDesc cubeResourceSetDesc{};
    cubeResourceSetDesc.bindings = {cubeUboBinding};
    rdrData.cubeResourceSet = ExpectOrThrow(gbgfx->CreateResourceSet(cubeResourceSetDesc));
}

struct AppData {
    audio::ALAudioStreamVorbisPlayer& music;
    audio::ALAudioStreamVorbis& musicStream;

    SharedPtr<std::atomic<float>> accTimeAtomic{nullptr};
    SharedPtr<gfx::ICommandList> cmdList{nullptr};
};

void UpdateMain(float dT, core::Application& app, void* userPtr) {
    auto appData = (AppData*) userPtr;

    appData->accTimeAtomic->fetch_add(dT);
#ifdef __AX_AUDIO_ALSOFT__
    appData->music.Tick(dT);
#endif
}

void InitMain(core::Application& app, void* userPtr) {
    auto appData = (AppData*) userPtr;

    auto wndThread = app.GetWindowThread();
    auto gfxThread = app.GetGraphicsThread();

    gfxThread->SetAutoPresent(true);
    gfxThread->CapFrames(1000.0f);

    auto gfx = app.GetGraphics();
    gfx->SetVSync(false);

    SharedPtr<std::atomic<float>> accTimeAtomic = std::make_shared<std::atomic<float>>(0.0f);
    appData->accTimeAtomic = accTimeAtomic;

    SharedPtr<gfx::ICommandList> cmdList = gfx->PrepCommandList();
    appData->cmdList = cmdList;

    SharedPtr<RenderData> rdrData = std::make_shared<RenderData>(wndThread, gfxThread, cmdList);
    gfxThread->EnqueueFuture([rdrData]() mutable {
        InitRender(*rdrData);
    }).get();
    gfxThread->CreateWork([rdrData]() mutable {
        UpdateRender(*rdrData);
    });

    SharedPtr<WindowData> wndData = std::make_shared<WindowData>(wndThread, accTimeAtomic);
    wndThread->CreateWork([wndData]() mutable {
        UpdateWnd(*wndData);
    });
    
#ifdef __AX_AUDIO_ALSOFT__
    appData->musicStream.Open().ThrowIfValid();

    appData->music.Play(&appData->musicStream);
    appData->music.SetLooping(true);
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

    auto ogg = ExpectOrThrow(audio::OGG_LoadFile("man_behind_slaughter.ogg"));
    audio::ALAudioStreamVorbis stream(ogg);

    auto wav = ExpectOrThrow(audio::WAV_LoadFile("test_mono.wav"));
    if (!buff.Load(wav)) {
        std::cerr << "Failed to load test_mono.wav audio onto ALAudioBuffer\n";
    }
    //soundCues.Play(buff);
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
        .fixedTickRate = ChMillis(1),
        .enforceGfxType = true,
        .enforcedGfxType = core::GfxType::GL330,
    };
    AppData userData {
        .music = music,
        .musicStream = stream
    };
    return app.InitCurrent(spec, InitMain, UpdateMain, &userData).code;
#else
    std::cerr << "Host is either not Win32 or doesn't support GL 330!\n";
    return -1;
#endif
}