#pragma once

#include "axle/core/concurrency/AX_ThreadCycler.hpp"

#include "AX_IGraphicsBackend.hpp"

namespace axle::gfx
{

std::size_t RPipeline_Hash_StencilOpState(const StencilOpState& s);
std::size_t RPipeline_Hash_DepthStencil(const DepthStencilState& d);
std::size_t RPipeline_Hash_Blend(const BlendState& b);
std::size_t RPipeline_Hash_Raster(const RasterState& r);
std::size_t RPipeline_Hash_VertexLayout(const MeshVertexLayout& layout);
std::size_t RPipeline_Hash_PipelineDesc(const RenderPipelineDesc& desc);

class PipelineManager {
private:
    SharedPtr<core::ThreadContextGfx> m_GfxThread;
    std::unordered_map<std::size_t, RenderPipelineHandle> m_PipelineLookup;

    RenderPipelineHandle Create(const RenderPipelineDesc& desc);
public:
    PipelineManager(SharedPtr<core::ThreadContextGfx> gfxThr);
    ~PipelineManager();

    RenderPipelineHandle GetOrCreate(const RenderPipelineDesc& desc);
};

}