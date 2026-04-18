// Application Context
#include "axle/core/window/AX_IWindow.hpp"

#include "axle/core/app/AX_Application.hpp"

// Render Context
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
// Graphics RenderContext
#include "axle/graphics/ctx/AX_IRenderContext.hpp"
// Graphics Scene
#include "axle/graphics/scene/AX_Camera.hpp"
// Graphics RenderLayer management (Sprites begin/draw/end call manager)
#include "axle/graphics/layer/AX_RenderLayer.hpp"
// Fonts
#include "axle/graphics/text/AX_BitmapFont.hpp"

#include "stb_image_write.h"

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
    Engine Foundation Specification

    Graphics Targets:
    - OpenGL Core 3.3 (primary)
    - OpenGL ES 3.0 shader compatibility
    - Future-ready architecture for Vulkan / D3D12
    - Optional compute shaders (HDR exposure, GPU particles, etc.)

    ============================================================
    CORE SYSTEMS
    ============================================================

    - [ ] Logging System
        -> [ ] BungeeCord-style minimalistic ANSI logger
        -> [ ] Pattern-ready formatting
        -> [ ] Timestamp support
        -> [ ] Auto-rotating log files
        -> [ ] Enumerated log levels
        -> [ ] Lock-free queue logging
        -> [ ] Reuse implementation from: /home/kalix/IdeaProjects/DockerClient

    - [ ] Memory System
        -> [ ] Linear allocator
        -> [ ] Pool allocator
        -> [ ] Frame allocator
        -> [ ] Debug allocation tracking
        -> [ ] GPU staging allocators

    - [ ] Time System
        -> [ ] High precision clock
        -> [ ] Fixed timestep simulation
        -> [ ] Variable render timestep
        -> [ ] Frame pacing utilities

    - [ ] Crash & Diagnostics
        -> [ ] Crash handler
        -> [ ] Stacktrace capture
        -> [ ] Minidump generation (platform dependent)

    ============================================================
    MULTITHREADING / JOB SYSTEM
    ============================================================

    - [*] ThreadCyclers
        -> [*] Context-ready worker threads
        -> [*] Task queues
        -> [*] Sort-aware work handles
        -> [*] Synchronized execution

    - [ ] JobSystem
        -> [ ] Fixed worker thread pool
        -> [ ] Work-stealing job queues
        -> [ ] JobHandle / Fence
        -> [ ] Dependency system
        -> [ ] ParallelFor utilities

    - [ ] Executors
        -> [ ] Fixed number of threads
        -> [ ] condition_variable scheduling
        -> [ ] NanoSleep precision waits
        -> [ ] async result propagation

    - [ ] AxTasks
        -> [ ] Built on ThreadCyclers
        -> [ ] Task handles
        -> [ ] Task manager
        -> [ ] Tick-based scheduling
        -> [ ] High precision NanoSleep internally

    ============================================================
    PLATFORM LAYER
    ============================================================

    - [ ] Monitor / Display Backend
        -> [ ] Win32 (first)
        -> [ ] Xlib / X11
        -> [ ] Cocoa

    - [ ] Window System
        -> [ ] WindowSpec descriptor
        -> [ ] Monitor selection
        -> [ ] Window modes:
                -> [ ] Windowed
                -> [ ] Borderless
                -> [ ] Fullscreen
        -> [ ] Alpha modes
        -> [ ] Popup / overlay windows
        -> [ ] Test all behaviors

    - [ ] Application Layer
        -> [ ] core::Application main entry
        -> [ ] Layer / LayerStack architecture
        -> [ ] Respect main-thread UI constraints
        -> [ ] Mobile / Web / TV ready structure

    ============================================================
    INPUT SYSTEM
    ============================================================

    - [ ] Raw input backend (mouse / keyboard)
    - [ ] Touch input support
    - [ ] Drag system for core::DiscreteState
    - [ ] Input abstraction layer
        -> [ ] Actions
        -> [ ] Axes
        -> [ ] Gamepads
        -> [ ] Device mapping

    ============================================================
    FILESYSTEM & ASSET PIPELINE
    ============================================================

    - [ ] Virtual File System (VFS)
        -> [ ] Mount points
        -> [ ] Directory + archive support
        -> [ ] Memory streams

    - [ ] FileDataStream
    - [ ] BufferDataStream

    - [ ] AssetLoader
        -> [ ] Supported formats:
            -> [ ] glTF
            -> [ ] OBJ
            -> [ ] FBX

        Responsibilities:

        -> Parse glTF JSON
        -> Map binary buffers (.bin) to BufferDataStream
        -> Load resources independently:
            - vertices
            - indices
            - textures
            - animations
            - skeleton data

        -> Texture formats
            * BCn / DXT
            * S3TC
            * ASTC
            * Basis Universal

        -> AssetPool (shared resources)
            NOTE:
            "Model" is only a logical grouping.
            Real resources are shared meshes.

        -> ModelHandle
            Contains:
                ModelID
                ModelName
                ModelExtension

        -> Scene hierarchy
            Instantiate Node structures
            Link nodes respecting hierarchy

        -> MeshInstances
            Shared mesh with different transforms

        -> MeshUploader
            Queued GPU uploads
            Render-thread execution
            Async error handling

    - [ ] Asset Hot Reload
        -> File watching
        -> Reload shaders / textures

    ============================================================
    RENDERING ARCHITECTURE
    ============================================================

    - [ ] GraphicsCapabilities
        -> Query host GPU features
        -> Extension detection
        -> Limit querying

    - [*] Graphics Command Interface
        GS Commands for:
            - Vertex buffers
            - Index buffers
            - Textures
            - Samplers
            - Depth / Stencil attachments

    - [ ] GPU Resource Manager
        -> Deferred destruction
        -> Frame fences
        -> Resource lifetime tracking

    - [ ] RenderCommandBuffer
        -> Multi-threaded command recording
        -> Render-thread submission
        -> State sorting

    ============================================================
    SHADER SYSTEM
    ============================================================

    - [ ] Shader pipeline

        Preferred pipeline:

            GLSL
              ↓
            glslang
              ↓
            SPIR-V
              ↓
            SPIRV-Cross
              ↓
            GLSL / HLSL backends

        Reason:
            Slang has heavy dependencies and portability issues.

    - [ ] Shader reflection
    - [ ] Pipeline caching
    - [ ] Descriptor layout generation

    ============================================================
    RENDER PROCEDURE (CORE RENDER ABSTRACTION)
    ============================================================

    - [ ] RenderProcedure

        Features:

        -> Validate GPU feature support
        -> Accept mesh input (vertex + index)
        -> Use ShaderHandle(s)
        -> Attach ResourceSetHandle(s)
        -> Output to attachments:
            - swapchain
            - backbuffer
            - textures

    ============================================================
    RENDERING TEST PIPELINE
    ============================================================

    Validate RenderProcedure with real scenes:

    - [ ] Blinn-Phong renderer
        Test scene:
            * Sponza
            * Damaged helmet

    - [ ] PBR renderer

    - [ ] Shadow system
        -> Depth map generation
        -> Attach as ResourceSet

    - [ ] GPU particle system
        -> Instanced draw
        -> UBO instance matrices
        -> Time-driven animation

    - [ ] Grass renderer
        -> Batched instancing
        -> Wind animation

    - [ ] Clustered Forward renderer

    - [ ] Deferred renderer
        -> GBuffer resources

    - [ ] Clustered Deferred renderer

    ============================================================
    BATCHING SYSTEMS
    ============================================================

    - [ ] BatchUtils

        -> Text batching
        -> Mesh batching
        -> Texture atlas batching

    - [ ] Batched rendering pipeline
        Input:
            AssetLoader AssetPool
        Output:
            Optimized draw submission

    ============================================================
    AUDIO
    ============================================================

    - [*] AudioStream stability
        -> Prevent queue starvation
        -> Loop fallback if buffer underruns
        -> Context-safe streaming

    ============================================================
    DEBUGGING / PROFILING
    ============================================================

    - [ ] CPU profiler zones
    - [ ] GPU timers
    - [ ] Frame timeline
    - [ ] DebugDraw system
        -> lines
        -> boxes
        -> spheres
        -> frustums

    ============================================================
    KNOWN ISSUES
    ============================================================

    - [*] Gameloop freeze on Stop()
        Possible deadlock
        Investigate thread shutdown ordering
*/


struct Uniforms {
    glm::mat4 MVP[3];
    glm::vec3 lightDir{glm::normalize(glm::vec3{-1.5f, -1.0f, -1.0f})};
    float lightShininess{8.0f};
    glm::vec3 lightColor{glm::vec3(1.0f, 1.0f, 1.0f)};
    float _pad0;
    glm::vec3 cameraPos{0.0f, 0.0f, 0.0f};
    float _pad1;

    Uniforms() {
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
    Uniforms(scene::Camera& camera, core::DiscreteState& state) {
        float fwidth = float(state.GetWidth());
        float fheight = float(state.GetHeight());

        if (fwidth == 0) fwidth = 1.0f;
        if (fheight == 0) fheight = 1.0f;

        MVP[0] = glm::mat4(1.0f);
        MVP[1] = camera.GetViewMatrix();
        MVP[2] = glm::perspective(glm::radians(70.0f), fwidth / fheight, 0.1f, 100.0f);

        cameraPos = camera.GetPosition();
    }
};

struct UBOState {
    Uniforms uniforms;
    bool reqUpdateUBO{true};

    Uniforms& Peek() {
        reqUpdateUBO = false;
        return uniforms;
    }
    void Update(scene::Camera& camera, core::DiscreteState& state) {
        reqUpdateUBO = true;
        uniforms = Uniforms(camera, state);
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

    //wndctx->SetAlphaConstant(0.75f + 0.25f * std::sin(0.65f * accTime));
}

struct RenderData {
    SharedPtr<core::ThreadContextWnd> wndThread;
    SharedPtr<core::ThreadContextGfx> gfxThread;
    SharedPtr<gfx::ICommandList> cmdList;
    SharedPtr<gfx::RenderLayer> renderLayers;

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

void UpdateCube(core::ThreadContextGfx* gfxThread, float dT, void* userPtr) {
    auto& rdrData = *(RenderData*)userPtr;

    auto gbgfx = gfxThread->GetContext();

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
        auto& uniforms = rdrData.uboState->Peek();
        gbgfx->UpdateBuffer(rdrData.cubeUniformBuffer, 0, sizeof(Uniforms), &uniforms).ThrowIfValid();
    }
}

void DrawCube(core::ThreadContextGfx* gfxThread, float dT, void* userPtr) {
    auto gbgfx = gfxThread->GetContext();
    auto& rdrData = *(RenderData*) userPtr;

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
    
    gfx::RenderPassClear clearArgs{};
    clearArgs.clearDepth = 1.0f;
    clearArgs.clearStencil = 0;
    clearArgs.clearColor[0] = clearArgs.clearColor[1] = clearArgs.clearColor[2] = 0.1f;
    clearArgs.clearColor[3] = 1.0f;
    // clearArgs.clearColor[0] = 0.5f + 0.5f * std::sin(accTime);
    // clearArgs.clearColor[1] = 0.5f + 0.5f * std::sin(accTime + 2.094f);
    // clearArgs.clearColor[2] = 0.5f + 0.5f * std::sin(accTime + 4.188f);

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
    auto layers = rdrData.renderLayers;

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
    shaderCubeDesc.entryPoints = {std::vector<gfx::ShaderEntryPoint>{shaderVertexEntry, shaderFragmentEntry}};

    auto shaderCube = ExpectOrThrow(gbgfx->CreateProgram(shaderCubeDesc));

    gfx::VertexLayout layout;
    gfx::VertexTypeDesc typeDesc {
        gfx::VertexAttributeClass::Float, 
        gfx::VertexAttributeType::Float32
    };

    uint32_t currentOffset = 0;
    const uint32_t FLOAT_SIZE = sizeof(float); // 4 bytes

    // 1. Position (float3, location 0)
    std::vector<gfx::VertexAttribute> attributes;
    attributes.push_back({0, 3, 0, currentOffset, FLOAT_SIZE * 3, typeDesc, false});
    currentOffset += FLOAT_SIZE * 3;
    // 2. Normal (float3, location 1)
    attributes.push_back({1, 3, 0, currentOffset, FLOAT_SIZE * 3, typeDesc, false});
    currentOffset += FLOAT_SIZE * 3;
    // 3. TexCoord (float2, location 2)
    attributes.push_back({2, 2, 0, currentOffset, FLOAT_SIZE * 2, typeDesc, false});
    currentOffset += FLOAT_SIZE * 2;

    layout.attributes = utils::CowSpan{attributes};
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
    cubeUboDesc.size = sizeof(Uniforms);
    cubeUboDesc.usage = gfx::BufferUsage::Uniform;
    cubeUboDesc.access = gfx::BufferAccess::Dynamic;
    cubeEboDesc.cpuVisible = true;
    rdrData.cubeUniformBuffer = ExpectOrThrow(gbgfx->CreateBuffer(cubeUboDesc));

    gfx::Binding cubeUboBinding{};
    cubeUboBinding.type = gfx::BindingType::UniformBuffer;
    cubeUboBinding.resource = gfx::ResourceHandle(rdrData.cubeUniformBuffer);
    cubeUboBinding.range = cubeUboDesc.size;

    gfx::ResourceSetDesc cubeResourceSetDesc{};
    cubeResourceSetDesc.bindings = {std::vector<gfx::Binding>{cubeUboBinding}};
    rdrData.cubeResourceSet = ExpectOrThrow(gbgfx->CreateResourceSet(cubeResourceSetDesc));

    gfx::RLDesc rlDesc = {
        .updateFunc = UpdateCube,
        .drawFunc = DrawCube,
        .stage = gfx::RLStage::Normal,
        .sortKey = 0
    };
    layers->CreateLayer(rlDesc);
    layers->RegisterWork(ExpectOrThrow(gbgfx->GetSwapchainFramebuffer(0)), rdrData.gfxThread);
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
    gfxThread->SetFrameCap(1000.0f);

    // gfx::BitmapFont fonts;
    // std::cout << "1\n";
    // fonts.LoadFont("comic.ttf");
    // std::cout << "2\n";
    // fonts.GenerateGlyphs({0, 128});
    // std::cout << "3\n";
    // fonts.TransformToPages();
    // std::cout << "4\n";

    // std::cout << "5\n";
    // const auto& page = utils::ExpectOrThrow(fonts.GetPage('A'));
    // stbi_write_png("atlas.png", page.width, page.height, 4, (const uint8_t*)page.buffer.data(), 0);
    // std::cout << "6\n";

    // throw std::runtime_error("break");

    auto gfx = app.GetGraphics();
    gfx->SetVSync(false);

    SharedPtr<std::atomic<float>> accTimeAtomic = std::make_shared<std::atomic<float>>(0.0f);
    appData->accTimeAtomic = accTimeAtomic;

    SharedPtr<gfx::ICommandList> cmdList = gfx->PrepCommandList();
    appData->cmdList = cmdList;

    SharedPtr<RenderData> rdrData = std::make_shared<RenderData>(wndThread, gfxThread, cmdList);
    auto layers = std::make_shared<gfx::RenderLayer>(rdrData);
    rdrData->renderLayers = layers;
    gfxThread->EnqueueFuture([rdrData]() mutable {
        InitRender(*rdrData);
    });

    SharedPtr<WindowData> wndData = std::make_shared<WindowData>(wndThread, accTimeAtomic);
    wndThread->CreateWork([wndData]() mutable {
        UpdateWnd(*wndData);
    });
    
#ifdef __AX_AUDIO_ALSOFT__
    appData->musicStream.Open().ThrowIfValid();

    appData->music.Play(&appData->musicStream);
    appData->music.SetGain(0.5f);
    appData->music.SetLooping(true);
#endif
}

int main() {
#if defined(__AX_PLATFORM_WIN32__) && defined(__AX_GRAPHICS_GL__)

#ifdef __AX_AUDIO_ALSOFT__
    audio::alc::CreateContext(nullptr).ThrowIfValid();

    audio::ALAudioStreamVorbisPlayer music(4);
    music.ApplyToSources([](audio::ALAudioStreamVorbisSource& src){
        src.BypassHRTF();
    });

    audio::ALAudioBufferPlayer soundCues(16);
    audio::ALAudioBuffer buff;

    auto ogg = ExpectOrThrow(audio::OGG_LoadFile("man_behind_slaughter.ogg"));
    audio::ALAudioStreamVorbis stream(ogg);

    auto wav = ExpectOrThrow(audio::WAV_LoadFile("test_mono.wav"));
    buff.Load(wav).ThrowIfValid();

    //soundCues.Play(buff);
#endif
    using namespace axle::gfx;

    core::Application app;
    core::ApplicationSpec spec {
        .wndspec {
            .title = "Test",
            .width = 800,
            .height = 600,
            .alphaMode = core::WndAlphaMode::None,
            .alphaModeColor = {0.1f, 0.1f, 0.1f},
            .resizable = true
        },
        .fixedTickRate = ChMillis(1),
        .enforceGfxType = true,
        .enforcedGfxType = gfx::GfxType::GL330,
        .surfaceDesc {
            .colors = gfx::SurfaceColors::R8G8B8A8_Unorm,
            .samples = gfx::SurfaceSamples::MSAA_4
        }
    };
    AppData userData {
        .music = music,
        .musicStream = stream
    };

    auto err = app.InitCurrent(spec, InitMain, UpdateMain, &userData);

    if (err.IsValid()) {
        std::cout << "Error: " << err.GetMessage() << std::endl;
        std::cout << "Press any key to continue..." << std::endl;
        char junk[1];
        std::cin.getline(junk, 1);
    }

    return err.GetCode();
#else
    std::cerr << "Host is either not Win32 or doesn't support GL 330!\n";
    return -1;
#endif
}