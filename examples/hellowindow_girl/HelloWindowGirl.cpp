#if defined(__AX_ASSETS_ASSIMP__)

#include "axle/assets/AX_AssetSTLAssimpFileImporter.hpp"

#include "axle/core/app/AX_Application.hpp"

#include "axle/graphics/scene/AX_Camera.hpp"

#include <iostream>

using namespace axle;

using namespace axle::assets;
using namespace axle::scene;
using namespace axle::core;
using namespace axle::gfx;

void Pause() {
    std::cerr << "Press any key to continue..." << std::endl;
    char junk[1];
    std::cin.getline(junk, 1);
}

struct Uniforms {
    glm::mat4 MVP[3];
    glm::vec3 lightColor{glm::vec3(1.0f, 1.0f, 1.0f)};
    float _pad0;
    glm::vec3 cameraPos{2.0f, 3.0f, 2.0f};
    float _pad1;
    glm::vec3 lightDir{glm::normalize(glm::vec3{1.0f, 0.5f, 1.0f})};
    uint32_t useNormalMap{0};

    Uniforms() {
        MVP[0] = glm::mat4(1.0f);
        MVP[1] = glm::lookAt(
            glm::vec3(0.5f, 2.0f, 2.0f),
            glm::vec3(0.0f, 0.75f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        MVP[2] = glm::perspective(
            glm::radians(70.0f),
            800.0f / 600.0f,
            0.1f,
            100.0f
        );
    }
    Uniforms(Camera& camera, DiscreteState& state) {
        float fwidth = float(state.GetWidth());
        float fheight = float(state.GetHeight());

        if (fwidth == 0) fwidth = 1.0f;
        if (fheight == 0) fheight = 1.0f;

        MVP[0] = glm::mat4(1.0f);
        MVP[1] = camera.GetViewMatrix();
        MVP[2] = glm::perspective(glm::radians(70.0f), fwidth / fheight, 0.1f, 100.0f);

        cameraPos = camera.GetPosition();
    }

    void UpdateLook(float dT) {
        static thread_local float accTime{0.0f};
        accTime += dT;

        MVP[1] = glm::lookAt(
            glm::vec3(2 * std::sin(accTime), 2.0f, 2 * std::cos(accTime)),
            glm::vec3(0.0f, 0.75f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
    }
};

struct UBOState {
    Uniforms uniforms;
    bool reqUpdateUBO{true};

    Uniforms& Peek() {
        reqUpdateUBO = false;
        return uniforms;
    }
    void Update(Camera& camera, DiscreteState& state) {
        reqUpdateUBO = true;
        uniforms = Uniforms(camera, state);
    }
};

struct RenderData {
    SharedPtr<AssetImportResult> asset_asf{nullptr};
    SharedPtr<DiscreteState> wndState{nullptr};
    SharedPtr<ThreadContextGfx> gfxThread{nullptr};
    SharedPtr<ICommandList> cmdList{nullptr};
    std::atomic<UBOState> uboState{};

    RenderPassHandle defPass;
    RenderPipelineHandle girlPipeline;
    BufferHandle girlVertices;
    BufferHandle girlIndices;
    BufferHandle girlUniforms;
    TextureHandle girlAlbedoMap;
    ResourceSetHandle girlResourceSet;
    uint32_t girlIndicesCount;
};

void Girl_Init(SharedPtr<RenderData> rdrData) {
    auto asset_asf = rdrData->asset_asf;
    auto gbgfx = rdrData->gfxThread->GetContext();

    Node node_ch03;
    for (auto& child : asset_asf->nodes[0].children) {
        if (child.name == "Ch03") {
            node_ch03 = child;
            break;
        }
    }

    AssetMesh& mesh_ch03 = asset_asf->meshes[node_ch03.meshId];
    AssetBuffer& mesh_ch03_vertices = asset_asf->buffers[mesh_ch03.vertexBufferIdx];
    AssetBuffer& mesh_ch03_indices = asset_asf->buffers[mesh_ch03.indexBufferIdx];

    DefaultRenderPassDesc drpdesc{};
    drpdesc.colorOps.load = LoadOp::Clear;
    drpdesc.colorOps.store = StoreOp::Store;
    drpdesc.depthStencilOps.load = LoadOp::Clear;
    drpdesc.depthStencilOps.store = StoreOp::Discard;
    rdrData->defPass = ExpectOrThrow(gbgfx->CreateDefaultRenderPass(drpdesc));

    ShaderDesc shaderGirlDesc;
    shaderGirlDesc.pipelineType = PipelineType::Graphics;
    shaderGirlDesc.sourcePath = "girl.slang";
    shaderGirlDesc.entryPointVertex = "mainVertex";
    shaderGirlDesc.entryPointFragment = "mainFragment";
    auto shaderGirl = ExpectOrThrow(gbgfx->CreateProgram(shaderGirlDesc));

    MeshVertexLayout layout;
    VertexTypeDesc typeDesc {
        VertexAttributeClass::Float, 
        VertexAttributeType::Float32
    };

    uint32_t currentOffset = 0;
    const uint32_t FLOAT_SIZE = sizeof(float); // 4 bytes

    // 1. Position (float3, POSITION0)
    std::vector<MeshVertexAttribute> attributes;
    attributes.push_back({VertexSemantic::Position, 0, 3, 0, currentOffset, FLOAT_SIZE * 3, typeDesc, false});
    currentOffset += FLOAT_SIZE * 3;
    // 2. Normal (float3, NORMAL0)
    attributes.push_back({VertexSemantic::Normal, 0, 3, 0, currentOffset, FLOAT_SIZE * 3, typeDesc, false});
    currentOffset += FLOAT_SIZE * 3;
    // 3. TexCoord (float2, TEXCOORD0)
    attributes.push_back({VertexSemantic::TexCoord, 0, 2, 0, currentOffset, FLOAT_SIZE * 2, typeDesc, false});
    currentOffset += FLOAT_SIZE * 2;
    // 4. Tangent (float3, TANGENT0)
    attributes.push_back({VertexSemantic::Tangent, 0, 3, 0, currentOffset, FLOAT_SIZE * 3, typeDesc, false});
    currentOffset += FLOAT_SIZE * 3;
    // 5. BiTangent (float3, BINORMAL0)
    attributes.push_back({VertexSemantic::Bitangent, 0, 3, 0, currentOffset, FLOAT_SIZE * 3, typeDesc, false});
    currentOffset += FLOAT_SIZE * 3;

    layout.attributes = utils::CowSpan{std::move(attributes)};
    layout.stride = currentOffset; 

    RenderPipelineDesc psoDesc{};
    psoDesc.renderPass = rdrData->defPass;
    psoDesc.shader = shaderGirl;
    psoDesc.vertexLayout = layout;
    psoDesc.raster.cull = CullMode::None;
    psoDesc.blend.enabled = true;
    rdrData->girlPipeline = ExpectOrThrow(gbgfx->CreateRenderPipeline(psoDesc));

    BufferDesc girlVboDesc{};
    girlVboDesc.size = mesh_ch03_vertices.raw.size();
    girlVboDesc.usage = BufferUsage::Vertex;
    girlVboDesc.access = BufferAccess::Immutable;
    girlVboDesc.cpuVisible = false;
    rdrData->girlVertices = ExpectOrThrow(gbgfx->CreateBuffer(girlVboDesc));
    gbgfx->UpdateBuffer(rdrData->girlVertices, 0, girlVboDesc.size, mesh_ch03_vertices.raw.data()).ThrowIfValid();

    BufferDesc girlEboDesc{};
    girlEboDesc.size = mesh_ch03_indices.raw.size();
    girlEboDesc.usage = BufferUsage::Index;
    girlEboDesc.access = BufferAccess::Immutable;
    girlEboDesc.cpuVisible = false;
    rdrData->girlIndices = ExpectOrThrow(gbgfx->CreateBuffer(girlEboDesc));
    rdrData->girlIndicesCount = mesh_ch03_indices.count;
    gbgfx->UpdateBuffer(rdrData->girlIndices, 0, girlEboDesc.size, mesh_ch03_indices.raw.data()).ThrowIfValid();

    BufferDesc girlUboDesc{};
    girlUboDesc.size = sizeof(Uniforms);
    girlUboDesc.usage = BufferUsage::Uniform;
    girlUboDesc.access = BufferAccess::Dynamic;
    girlUboDesc.cpuVisible = true;
    rdrData->girlUniforms = ExpectOrThrow(gbgfx->CreateBuffer(girlUboDesc));

    auto& girlAlbedoImg = asset_asf->textures[0].image;

    TextureSubDesc girlAlbedoSubDesc{};
    girlAlbedoSubDesc.generateMips = true;
    girlAlbedoSubDesc.aniso = 4;
    girlAlbedoSubDesc.mipFilter = MipmapFilter::Linear;
    girlAlbedoSubDesc.minFilter = TextureFilter::Linear;
    girlAlbedoSubDesc.magFilter = TextureFilter::Linear;

    TextureDesc girlAlbedoDesc{};
    girlAlbedoDesc.type = TextureType::Texture2D;
    girlAlbedoDesc.width = girlAlbedoImg.width;
    girlAlbedoDesc.height = girlAlbedoImg.height;
    girlAlbedoDesc.samples = SampleCount::Sample1;
    girlAlbedoDesc.format =  TextureFormat::RGB8_UNORM;
    girlAlbedoDesc.usage = TextureUsage::Sampled;
    girlAlbedoDesc.subDesc = girlAlbedoSubDesc;
    girlAlbedoDesc.pixelsByLayers = {std::vector<utils::URaw>{girlAlbedoImg.bytes}};
    rdrData->girlAlbedoMap = ExpectOrThrow(gbgfx->CreateTexture(girlAlbedoDesc));

    Binding girlUboBinding{};
    girlUboBinding.type = BindingType::UniformBuffer;
    girlUboBinding.resource = ResourceHandle(rdrData->girlUniforms);
    girlUboBinding.range = girlUboDesc.size;

    Binding girlAlbedoBinding{};
    girlAlbedoBinding.type = BindingType::SampledTexture;
    girlAlbedoBinding.resource = ResourceHandle(rdrData->girlAlbedoMap);
    girlAlbedoBinding.slot = 1;

    ResourceSetDesc girlResourceSetDesc{};
    girlResourceSetDesc.bindings = {std::vector<Binding>{girlUboBinding, girlAlbedoBinding}};
    rdrData->girlResourceSet = ExpectOrThrow(gbgfx->CreateResourceSet(girlResourceSetDesc));
}

void Girl_Draw(SharedPtr<RenderData> rdrData) {
    auto gfxThread = rdrData->gfxThread;
    auto gbgfx = gfxThread->GetContext();

    auto uboState = rdrData->uboState.load();
    auto ubo = uboState.Peek();

    ubo.UpdateLook(gfxThread->GetFrameTime());
    gbgfx->UpdateBuffer(rdrData->girlUniforms, 0, sizeof(Uniforms), &ubo).ThrowIfValid();

    uint32_t imgIndex = ExpectOrThrow(gbgfx->AcquireNextImage());

    auto cmdList = rdrData->cmdList;
    cmdList->Begin();
    cmdList->SetViewport({0.0f, 0.0f, 1024.0f, 768.0f});
    
    RenderPassClear clearArgs{};
    clearArgs.clearDepth = 1.0f;
    clearArgs.clearStencil = 0;
    clearArgs.clearColor[0] = clearArgs.clearColor[1] = clearArgs.clearColor[2] = 0.1f;
    clearArgs.clearColor[3] = 1.0f;

    cmdList->BeginRenderPass({rdrData->defPass, ExpectOrThrow(gbgfx->GetSwapchainFramebuffer(imgIndex)), clearArgs});
    cmdList->BindRenderPipeline({rdrData->girlPipeline});
    cmdList->BindVertexBuffer({rdrData->girlVertices});
    cmdList->BindIndexBuffer({rdrData->girlIndices});
    cmdList->BindResourceSet({rdrData->girlResourceSet});
    cmdList->DrawIndexed({rdrData->girlIndicesCount, 0});
    cmdList->EndRenderPass({rdrData->defPass});
    cmdList->End();

    gbgfx->Execute(*cmdList.get()).ThrowIfValid();
}

void App_Init(axle::core::Application &app, void *user) {
    auto wndThread = app.GetWindowThread();
    auto gfxThread = app.GetGraphicsThread();

    auto asset_asf = (AssetImportResult*)user;
    auto rdrData = std::make_shared<RenderData>(
        SharedPtr<AssetImportResult>{asset_asf},
        wndThread->GetContext()->GetDiscreteState(),
        gfxThread,
        ICommandList::Create(gfxThread)
    );
    gfxThread->SetAutoPresent(true);
    gfxThread->EnqueueTask([rdrData](){ Girl_Init(rdrData); });
    gfxThread->CreateWork([rdrData](){ Girl_Draw(rdrData); });
}

int main() {
    AssetImportDesc desc;
    desc.flags |= (uint32_t)AssetImportFlag::CalcTangents;
    desc.flags |= (uint32_t)AssetImportFlag::IncludePBR;

    AssetSTLAssimpFileImporter importer(desc, "asf.gltf");

    auto import = importer.Import();
    if (!import.has_value()) {
        auto err = import.error();
        std::cerr << err.GetMessage() << std::endl;
        Pause();
        return err.GetCode();
    }

    auto asset_asf = import.value();

    ApplicationSpec appSpec;
    appSpec.fixedTickRate = ChMillis(50);
    appSpec.fixedWndRate = ChNanos(1000);

    appSpec.surfaceDesc.colors = SurfaceColors::R8G8B8A8_sRGB;
    appSpec.surfaceDesc.samples = SurfaceSamples::MSAA_4;
    appSpec.surfaceDesc.vsync = false;

    appSpec.wndspec.width = 1024;
    appSpec.wndspec.height = 768;
    appSpec.wndspec.className = "HelloWindowGirl";
    appSpec.wndspec.title = "HelloWindowGirl";
    appSpec.wndspec.waitForNextEvent = true;

    auto empty = [](float, axle::core::Application&, void*){};
    Application app;
    auto appErr = app.InitCurrent(appSpec, App_Init, empty, &asset_asf);

    if (appErr.IsValid()) {
        std::cerr << "App Error: " << appErr.GetMessage() << std::endl;
        Pause();
    }
    return appErr.GetCode();
}

#endif