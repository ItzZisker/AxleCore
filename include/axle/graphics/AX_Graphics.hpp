#pragma once

#include "axle/core/concurrency/AX_ThreadCycler.hpp"

#include "axle/graphics/ctx/AX_IRenderContext.hpp"
#include "axle/graphics/cmd/AX_IGraphicsBackend.hpp"
#include "axle/graphics/cmd/AX_ICommandList.hpp"

#include "axle/utils/AX_Expected.hpp"
#include "axle/utils/AX_Types.hpp"

#include <mutex>

namespace axle::gfx {

class Graphics {
private:
    SharedPtr<core::ThreadContextGfx> m_GfxThread{nullptr};
    SharedPtr<IGraphicsBackend> m_GfxBackend{nullptr};

    GfxType m_GfxType{GfxType::GL330};

    std::mutex m_GfxCapsMutex{};
public:
    explicit Graphics(SharedPtr<core::ThreadContextGfx> gfxThread);

    // Swapchain / presentation control
    void SetVSync(bool enabled);

    // Resource creation (thread-safe, async)
    Future<utils::ExResult<BufferHandle>> CreateBuffer(const BufferDesc& desc);
    Future<utils::ExError> UpdateBuffer(const BufferHandle& handle, size_t offset, size_t size, const void* data);
    Future<utils::ExError> DestroyBuffer(const BufferHandle& handle);

    Future<utils::ExResult<TextureHandle>> CreateTexture(const TextureDesc& desc);
    Future<utils::ExError> UpdateTexture(const TextureHandle& handle, const TextureSubDesc& subDesc, const void* data);
    Future<utils::ExError> DestroyTexture(const TextureHandle& handle);

    Future<utils::ExResult<FramebufferHandle>> CreateFramebuffer(const FramebufferDesc&);
    Future<utils::ExError> DestroyFramebuffer(const FramebufferHandle& handle);

    Future<utils::ExResult<ShaderHandle>> CreateProgram(const ShaderDesc& desc);
    Future<utils::ExError> DestroyProgram(const ShaderHandle& handle);

    Future<utils::ExResult<RenderPipelineHandle>> CreateRenderPipeline(const RenderPipelineDesc& desc);
    Future<utils::ExError> DestroyRenderPipeline(const RenderPipelineHandle& handle);

    Future<utils::ExResult<ComputePipelineHandle>> CreateComputePipeline(const ComputePipelineDesc& desc);
    Future<utils::ExError> DestroyComputePipeline(const ComputePipelineHandle& handle);

    Future<utils::ExResult<RenderPassHandle>> CreateDefaultRenderPass(const DefaultRenderPassDesc& desc);
    Future<utils::ExResult<RenderPassHandle>> CreateRenderPass(const RenderPassDesc& desc);
    Future<utils::ExError> DestroyRenderPass(const RenderPassHandle& handle);

    Future<utils::ExResult<ResourceSetHandle>> CreateResourceSet(const ResourceSetDesc& desc);
    Future<utils::ExError> UpdateResourceSet(const ResourceSetHandle& handle, std::vector<Binding> bindings);
    Future<utils::ExError> DestroyResourceSet(const ResourceSetHandle& handle);

    Future<utils::ExResult<uint32_t>> AcquireNextImage();
    Future<utils::ExResult<FramebufferHandle>> GetSwapchainFramebuffer(uint32_t imageIndex);
    Future<utils::ExError> Present(uint32_t imageIndex);

    // Command recording (thread-safe, CPU-side)
    SharedPtr<ICommandList> PrepCommandList();
    Future<utils::ExError> Submit(SharedPtr<ICommandList> cmdList);

    // Capabilities (cached, immutable)
    const GraphicsCaps& Capabilities();
};


}