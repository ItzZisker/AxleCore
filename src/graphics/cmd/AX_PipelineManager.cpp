#include "axle/graphics/cmd/AX_PipelineManager.hpp"

#include "axle/utils/AX_Universal.hpp"

using namespace axle::utils;

namespace axle::gfx
{

PipelineManager::PipelineManager(SharedPtr<core::ThreadContextGfx> gfxThr)
    : m_GfxThread(gfxThr) {}

PipelineManager::~PipelineManager() {
    auto lambdaFinalizer = [
        pipelineLookup = std::move(m_PipelineLookup),
        gfxThread = m_GfxThread
    ]() {
        auto backend = gfxThread->GetContext();
        for (auto& [_, h] : pipelineLookup) {
            backend->DestroyRenderPipeline(h);
        }
    };
    if (m_GfxThread->ValidateThread()) {
        lambdaFinalizer();
    } else {
        m_GfxThread->EnqueueTask(lambdaFinalizer);
    }
}

ExResult<RenderPipelineHandle> PipelineManager::Create(const RenderPipelineDesc& desc) {
    auto lambdaCreator = [
        descCpy = desc,
        gfxThread = m_GfxThread
    ]() -> ExResult<RenderPipelineHandle> {
        auto backend = gfxThread->GetContext();
        return backend->CreateRenderPipeline(descCpy);
    };
    if (m_GfxThread->ValidateThread()) {
        return lambdaCreator();
    } else {
        return m_GfxThread->EnqueueFuture(lambdaCreator).get();
    }
}

ExResult<RenderPipelineHandle> PipelineManager::GetOrCreate(const RenderPipelineDesc& desc) {
    const size_t hash = RPipeline_Hash_PipelineDesc(desc);

    auto it = m_PipelineLookup.find(hash);
    if (it != m_PipelineLookup.end())
        return it->second;

    AX_DECL_OR_PROPAGATE(handle, PipelineManager::Create(desc));
    m_PipelineLookup[hash] = handle;

    return handle;
}

std::size_t RPipeline_Hash_StencilOpState(const StencilOpState& s) {
    size_t h = 0;
    HashEnum(h, s.compare);
    HashCombine(h, std::hash<uint32_t>{}(s.compareMask));
    HashCombine(h, std::hash<uint32_t>{}(s.writeMask));
    HashCombine(h, std::hash<uint32_t>{}(s.reference));
    HashEnum(h, s.failOp);
    HashEnum(h, s.depthFailOp);
    HashEnum(h, s.passOp);
    return h;
}

std::size_t RPipeline_Hash_DepthStencil(const DepthStencilState& d) {
    size_t h = 0;
    HashCombine(h, std::hash<bool>{}(d.depthTest));
    HashCombine(h, std::hash<bool>{}(d.depthWrite));
    HashEnum(h, d.depthCompare);
    HashCombine(h, std::hash<bool>{}(d.stencilTest));
    HashCombine(h, RPipeline_Hash_StencilOpState(d.stencilFront));
    HashCombine(h, RPipeline_Hash_StencilOpState(d.stencilBack));
    return h;
}

std::size_t RPipeline_Hash_Blend(const BlendState& b) {
    size_t h = 0;
    HashCombine(h, std::hash<bool>{}(b.enabled));
    HashEnum(h, b.srcColor);
    HashEnum(h, b.dstColor);
    HashEnum(h, b.colorOp);
    HashEnum(h, b.srcAlpha);
    HashEnum(h, b.dstAlpha);
    HashEnum(h, b.alphaOp);
    return h;
}

std::size_t RPipeline_Hash_Raster(const RasterState& r) {
    size_t h = 0;
    HashEnum(h, r.cull);
    HashEnum(h, r.fill);
    HashEnum(h, r.frontFace);
    HashEnum(h, r.polyMode);
    return h;
}

std::size_t RPipeline_Hash_VertexLayout(const MeshVertexLayout& layout) {
    size_t h = 0;
    HashCombine(h, std::hash<uint32_t>{}(layout.stride));
    for (const auto& a : layout.attributes) {
        HashEnum(h, a.semantic);
        HashCombine(h, std::hash<uint32_t>{}(a.semanticIndex));
        HashCombine(h, std::hash<uint32_t>{}(a.componentCount));
        HashCombine(h, std::hash<uint32_t>{}(a.divisor));
        HashCombine(h, std::hash<uint32_t>{}(a.offset));
        HashCombine(h, std::hash<uint32_t>{}(a.size));
        HashEnum(h, a.typeDesc._class);
        HashEnum(h, a.typeDesc.type);
        HashCombine(h, std::hash<bool>{}(a.normalized));
    }
    return h;
}

std::size_t RPipeline_Hash_PipelineDesc(const RenderPipelineDesc& desc) {
    size_t h = 0;
    HashCombine(h, std::hash<uint32_t>{}(desc.renderPass.index));
    HashCombine(h, std::hash<uint32_t>{}(desc.renderPass.generation));
    HashCombine(h, std::hash<uint32_t>{}(desc.shader.index));
    HashCombine(h, std::hash<uint32_t>{}(desc.shader.generation));
    HashCombine(h, RPipeline_Hash_VertexLayout(desc.vertexLayout));
    HashCombine(h, RPipeline_Hash_Raster(desc.raster));
    HashCombine(h, RPipeline_Hash_DepthStencil(desc.depth));
    HashCombine(h, RPipeline_Hash_Blend(desc.blend));
    return h;
}

}