#pragma once

#include "axle/core/concurrency/AX_ThreadCycler.hpp"
#include "axle/utils/AX_Expected.hpp"

#include "AX_IGraphicsBackend.hpp"

namespace axle::gfx
{

std::size_t RPipeline_Hash_StencilOpState(const StencilOpState& s);
std::size_t RPipeline_Hash_DepthStencil(const DepthStencilState& d);
std::size_t RPipeline_Hash_Blend(const BlendState& b);
std::size_t RPipeline_Hash_Raster(const RasterState& r);
std::size_t RPipeline_Hash_VertexLayout(const MeshVertexLayout& layout);
std::size_t RPipeline_Hash_PipelineDesc(const RenderPipelineDesc& desc);

class PipelineManager : AX_THR_RENDER_OWNED {
private:
    std::unordered_map<std::size_t, RenderPipelineHandle> m_PipelineLookup;

    utils::ExResult<RenderPipelineHandle> Create(const RenderPipelineDesc& desc);
public:
    PipelineManager(ThreadGfxScope gfxThr);
    ~PipelineManager();

    AX_NON_COPYABLE_NON_MOVABLE(PipelineManager);

    ThreadInvocation<utils::ExResult<RenderPipelineHandle>> GetOrCreate(const RenderPipelineDesc& desc);
    
    ThreadInvocation<utils::ExResult<RenderPipelineDesc>> Describe(const RenderPipelineHandle& handle);
    ThreadInvocation<utils::ExResult<RenderPipelineDesc>> Describe(std::size_t pipelineHash);

    ThreadInvocation<utils::ExError> Destroy(const RenderPipelineHandle& handle);
    ThreadInvocation<utils::ExError> Destroy(std::size_t pipelineHash);
};

}