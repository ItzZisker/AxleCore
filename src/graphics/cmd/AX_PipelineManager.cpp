#include "axle/graphics/cmd/AX_PipelineManager.hpp"

#include "axle/utils/AX_Universal.hpp"

using namespace axle::utils;

namespace axle::gfx
{

// class PipelineManager {
// private:
//     SharedPtr<IGraphicsBackend> m_GfxBackend;
//     std::unordered_map<std::size_t, RenderPipelineHandle> m_PipelineLookup;

//     RenderPipelineHandle Create(const RenderPipelineDesc& desc);
// public:
//     PipelineManager(SharedPtr<IGraphicsBackend> gfxBackend);
//     ~PipelineManager();

//     RenderPipelineHandle GetOrCreate(const RenderPipelineDesc& desc);
// };

PipelineManager::PipelineManager(SharedPtr<core::ThreadContextGfx> gfxThr) : m_GfxThread(gfxThr) {}
PipelineManager::~PipelineManager() {
    m_GfxThread->EnqueueTask([
        pipelineLookup = std::move(m_PipelineLookup),
        gfxBackend = m_GfxThread->GetContext()
    ]() {
        for (auto& [_, h] : pipelineLookup) {
            gfxBackend->DestroyRenderPipeline(h);
        }
    });
}

std::size_t RPipeline_Hash_StencilOpState(const StencilOpState& s) {
    size_t h = 0;
    HashEnum(h, s.compare);
    HashValue(h, s.compareMask);
    HashValue(h, s.writeMask);
    HashValue(h, s.reference);
    HashEnum(h, s.failOp);
    HashEnum(h, s.depthFailOp);
    HashEnum(h, s.passOp);
    return h;
}

std::size_t RPipeline_Hash_DepthStencil(const DepthStencilState& d) {
    size_t h = 0;
    HashValue(h, d.depthTest);
    HashValue(h, d.depthWrite);
    HashEnum(h, d.depthCompare);
    HashValue(h, d.stencilTest);
    HashCombine(h, RPipeline_Hash_StencilOpState(d.stencilFront));
    HashCombine(h, RPipeline_Hash_StencilOpState(d.stencilBack));
    return h;
}

std::size_t RPipeline_Hash_Blend(const BlendState& b) {
    size_t h = 0;
    HashValue(h, b.enabled);
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

std::size_t RPipeline_Hash_VertexLayout(const VertexLayout& layout) {
    size_t h = 0;
    HashValue(h, layout.stride);
    for (const auto& a : layout.attributes) {
        HashValue(h, a.location);
        HashValue(h, a.componentCount);
        HashValue(h, a.divisor);
        HashValue(h, a.offset);
        HashValue(h, a.size);
        HashEnum(h, a.typeDesc._class);
        HashEnum(h, a.typeDesc.type);
        HashValue(h, a.normalized);
    }
    return h;
}

std::size_t RPipeline_Hash_PipelineDesc(const RenderPipelineDesc& desc) {
    size_t h = 0;
    HashValue(h, desc.renderPass);
    HashValue(h, desc.shader);
    HashCombine(h, RPipeline_Hash_VertexLayout(desc.layout));
    HashCombine(h, RPipeline_Hash_Raster(desc.raster));
    HashCombine(h, RPipeline_Hash_DepthStencil(desc.depth));
    HashCombine(h, RPipeline_Hash_Blend(desc.blend));
    return h;
}

}