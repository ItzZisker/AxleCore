#pragma once

#include "axle/core/concurrency/AX_ThreadCycler.hpp"

#include "axle/graphics/ctx/AX_IRenderContext.hpp"
#include "axle/graphics/AX_GraphicsParams.hpp"

#include "axle/utils/AX_Expected.hpp"
#include "axle/utils/AX_Types.hpp"

namespace axle::gfx {

struct GraphicsBackendDesc {
    gfx::IRenderContext* nonowning_context;
    ThreadGfxScope gfxThread;
};

class IGraphicsBackend : AX_THR_RENDER_OWNED {
public:
    IGraphicsBackend(const GraphicsBackendDesc& desc) : ThreadOwned(desc.gfxThread) {}
    virtual ~IGraphicsBackend() = default;

    AX_NON_COPYABLE_NON_MOVABLE(IGraphicsBackend);

    virtual SharedPtr<gfx::IRenderContext> GetContext() const = 0;

    virtual bool SupportsCap(GraphicsCapEnum cap) = 0;
    virtual const GraphicsCaps& GetCaps() const = 0;
    virtual utils::ExResult<GraphicsCaps> QueryCaps() = 0;

    virtual utils::ExResult<SwapchainHandle> CreateSwapchain(const SwapchainDesc& desc) = 0;
    virtual utils::ExError DestroySwapchain(const SwapchainHandle& desc) = 0;
    virtual utils::ExError ResizeSwapchain(const SwapchainHandle& desc, uint32_t width, uint32_t height) = 0;
    virtual utils::ExResult<SwapchainDesc> DescribeSwapchain(const SwapchainHandle& handle) = 0;

    virtual utils::ExResult<BufferHandle> CreateBuffer(const BufferDesc& desc) = 0;
    virtual utils::ExError UpdateBuffer(const BufferHandle& handle, size_t offset, size_t size, const void* data) = 0;
    virtual utils::ExError DestroyBuffer(const BufferHandle& handle) = 0;
    virtual utils::ExResult<BufferDesc> DescribeBuffer(const BufferHandle& handle) = 0;

    virtual utils::ExResult<TextureHandle> CreateTexture(const TextureDesc& desc) = 0;
    virtual utils::ExError GenerateMipMaps(const TextureHandle& handle) = 0;
    virtual utils::ExError UpdateTexture(const TextureHandle& handle, const void* data) = 0;
    virtual utils::ExError DestroyTexture(const TextureHandle& handle) = 0;
    virtual utils::ExResult<TextureDesc> DescribeTexture(const TextureHandle& handle) = 0;

    virtual utils::ExResult<SamplerHandle> CreateSampler(const SamplerDesc& desc) = 0;
    virtual utils::ExError UpdateSampler(const SamplerHandle& handle, const SamplerDesc& subDesc) = 0;
    virtual utils::ExError DestroySampler(const SamplerHandle& handle) = 0;
    virtual utils::ExResult<SamplerDesc> DescribeSampler(const SamplerHandle& handle) = 0;

    virtual utils::ExResult<FramebufferHandle> CreateFramebuffer(const FramebufferDesc& handle) = 0;
    virtual utils::ExError DestroyFramebuffer(const FramebufferHandle& handle) = 0;
    virtual utils::ExResult<FramebufferDesc> DescribeFramebuffer(const FramebufferHandle& handle) = 0;

    virtual utils::ExResult<ShaderHandle> CreateProgram(const ShaderDesc& desc, ShaderInputState& out_VertexBindingsDesc) = 0;
    virtual utils::ExError DestroyProgram(const ShaderHandle& handle) = 0;
    virtual utils::ExResult<ShaderDesc> DescribeProgram(const ShaderHandle& handle) = 0;
    
    virtual utils::ExResult<RenderPipelineHandle> CreateRenderPipeline(const RenderPipelineDesc& desc) = 0;
    virtual utils::ExError DestroyRenderPipeline(const RenderPipelineHandle& handle) = 0;
    virtual utils::ExResult<RenderPipelineDesc> DescribeRenderPipeline(const RenderPipelineHandle& handle) = 0;
    
    virtual utils::ExResult<ComputePipelineHandle> CreateComputePipeline(const ComputePipelineDesc& desc) = 0;
    virtual utils::ExError DestroyComputePipeline(const ComputePipelineHandle& handle) = 0;
    virtual utils::ExResult<ComputePipelineDesc> DescribeComputePipeline(const ComputePipelineHandle& handle) = 0;

    virtual utils::ExResult<RenderPassHandle> CreateDefaultRenderPass(const DefaultRenderPassDesc& desc) = 0;
    virtual utils::ExResult<RenderPassHandle> CreateRenderPass(const RenderPassDesc& desc) = 0;
    virtual utils::ExError DestroyRenderPass(const RenderPassHandle& handle) = 0;
    virtual utils::ExResult<RenderPassDesc> DescribeRenderPass(const RenderPassHandle& handle) = 0;

    virtual utils::ExResult<ResourceSetHandle> CreateResourceSet(const ResourceSetDesc& desc) = 0;
    virtual utils::ExError UpdateResourceSet(const ResourceSetHandle& handle, const std::vector<ResourceBinding>& bindings) = 0;
    virtual utils::ExError DestroyResourceSet(const ResourceSetHandle& handle) = 0;
    virtual utils::ExResult<ResourceSetDesc> DescribeResourceSet(const ResourceSetHandle& handle) = 0;

    virtual utils::ExError Execute(ICommandList& cmd) = 0;
    virtual utils::ExError Dispatch(ICommandList& cmdlist, uint32_t x, uint32_t y, uint32_t z) = 0;
    virtual utils::ExError Barrier(ICommandList& cmdlist, std::vector<ResourceTransition> transitions) = 0;

    virtual utils::ExResult<uint32_t> AcquireNextImage() = 0;
    virtual utils::ExResult<FramebufferHandle> GetSwapchainFramebuffer(uint32_t imageIndex) = 0;
    virtual utils::ExError Present(uint32_t imageIndex) = 0;
};

}