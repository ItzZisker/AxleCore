#pragma once

#include "axle/graphics/cmd/AX_PipelineManager.hpp"
#include "axle/graphics/layer/AX_RenderProcedure.hpp"

#include "axle/utils/AX_Expected.hpp"

namespace axle::gfx
{

enum class RenderProcedureType {
    DirectDraw,
    DirectInstancedDraw,
    IndirectDraw
};

enum class TransformInput {
    VertexInput,
    Uniforms
};

struct RenderItemBuilderDesc {
    RenderProcedureType procedureType{RenderProcedureType::DirectDraw};
    TransformInput transformInput{TransformInput::VertexInput};
    bool separateSamplersFromTextures{false}; // Requires FeatureSet on Host device
};

class RenderItemBuilder {
private:
    SharedPtr<core::ThreadContextGfx> m_GfxThread;
    PipelineManager m_Pipelines;

    std::unordered_map<BufferHandle, BufferDesc>            m_TrackingBuffers;
    std::unordered_map<TextureHandle, TextureDesc>          m_TrackingTextures;
    std::unordered_map<ResourceSetHandle, ResourceSetDesc>  m_TrackingResources;

    // yejoori State haro sort kon inja negah dar ke vaghti Build gerefti, kamtarin State-Change hengam Draw eemal beshe

    // std::vector<ModelInstance> m_ModelInstances;

    // Reference Scene/Scene2D
    // Keep Model Instances here
public:
    RenderItemBuilder(SharedPtr<core::ThreadContextGfx> gfxThr, const RenderItemBuilderDesc& desc);
    ~RenderItemBuilder(); // Discard Handles, etc.

    // Add/Remove/Manage Model Instances
    // Everytime we add/remove, we shall call:
    // - Build: To Build new handles for new RenderItems for our RenderProcedure
    // - BuildAndDiscard: Discard previous handles and modify/create-new-ones for our RenderProcedure

    utils::CowSpan<RenderItem> BuildAndDiscard();
    utils::CowSpan<RenderItem> Build();
};

}