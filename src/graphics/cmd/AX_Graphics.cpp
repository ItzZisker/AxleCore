#include "axle/graphics/AX_Graphics.hpp"

#include "axle/graphics/cmd/GL/AX_GLCommandList.hpp"

#ifdef __AX_GRAPHICS_GL__
#include "axle/graphics/cmd/GL/AX_GLGraphicsBackend.hpp"
#endif

#include "axle/utils/AX_Expected.hpp"
#include "axle/utils/AX_Types.hpp"

#include <memory>
#include <mutex>

using namespace axle::utils;

namespace axle::gfx
{

Graphics::Graphics(SharedPtr<core::ThreadContextGfx> gfxThread) : m_GfxThread(gfxThread) {
    m_GfxBackend = gfxThread->GetContext();
}

void Graphics::SetVSync(bool enabled) {
    m_GfxThread->EnqueueTask([this, enabled = enabled]{
        auto ctx = m_GfxThread->GetContext();
        ctx->GetContext()->SetVSync(enabled);
    });
}

Future<ExResult<BufferHandle>> Graphics::CreateBuffer(const BufferDesc& desc) {
    return m_GfxThread->EnqueueFuture([this, desc = desc](){
        return m_GfxBackend->CreateBuffer(desc);
    });
}

Future<ExError> Graphics::UpdateBuffer(const BufferHandle& handle, size_t offset, size_t size, const void* data) {
    return m_GfxThread->EnqueueFuture([this, 
        handle = handle, offset = offset,
        size = size, data = data
    ](){
        return m_GfxBackend->UpdateBuffer(handle, offset, size, data);
    });
}

Future<ExError> Graphics::DestroyBuffer(const BufferHandle& handle) {
    return m_GfxThread->EnqueueFuture([this, handle = handle](){
        return m_GfxBackend->DestroyBuffer(handle);
    });
}

Future<ExResult<TextureHandle>> Graphics::CreateTexture(const TextureDesc& desc) {
    return m_GfxThread->EnqueueFuture([this, desc = desc](){
        return m_GfxBackend->CreateTexture(desc);
    });
}

Future<ExError> Graphics::UpdateTexture(const TextureHandle& handle, const TextureSubDesc& subDesc, const void* data) {
    return m_GfxThread->EnqueueFuture([this, handle = handle, subDesc = subDesc, data = data](){
        return m_GfxBackend->UpdateTexture(handle, subDesc, data);
    });
}

Future<ExError> Graphics::DestroyTexture(const TextureHandle& handle) {
    return m_GfxThread->EnqueueFuture([this, handle = handle](){
        return m_GfxBackend->DestroyTexture(handle);
    });
}

Future<ExResult<FramebufferHandle>> Graphics::CreateFramebuffer(const FramebufferDesc& desc) {
    return m_GfxThread->EnqueueFuture([this, desc = desc](){
        return m_GfxBackend->CreateFramebuffer(desc);
    });
}

Future<ExError> Graphics::DestroyFramebuffer(const FramebufferHandle& handle) {
    return m_GfxThread->EnqueueFuture([this, handle = handle](){
        return m_GfxBackend->DestroyFramebuffer(handle);
    });
}

Future<ExResult<ShaderHandle>> Graphics::CreateProgram(const ShaderDesc& desc) {
    return m_GfxThread->EnqueueFuture([this, desc = desc](){
        return m_GfxBackend->CreateProgram(desc);
    });
}

Future<ExError> Graphics::DestroyProgram(const ShaderHandle& handle) {
    return m_GfxThread->EnqueueFuture([this, handle = handle](){
        return m_GfxBackend->DestroyProgram(handle);
    });
}

Future<ExResult<RenderPipelineHandle>> Graphics::CreateRenderPipeline(const RenderPipelineDesc& desc) {
    return m_GfxThread->EnqueueFuture([this, desc = desc](){
        return m_GfxBackend->CreateRenderPipeline(desc);
    });
}

Future<ExError> Graphics::DestroyRenderPipeline(const RenderPipelineHandle& handle) {
    return m_GfxThread->EnqueueFuture([this, handle = handle](){
        return m_GfxBackend->DestroyRenderPipeline(handle);
    });
}

Future<ExResult<ComputePipelineHandle>> Graphics::CreateComputePipeline(const ComputePipelineDesc& desc) {
    return m_GfxThread->EnqueueFuture([this, desc = desc](){
        return m_GfxBackend->CreateComputePipeline(desc);
    });
}

Future<ExError> Graphics::DestroyComputePipeline(const ComputePipelineHandle& handle) {
    return m_GfxThread->EnqueueFuture([this, handle = handle](){
        return m_GfxBackend->DestroyComputePipeline(handle);
    });
}

Future<ExResult<RenderPassHandle>> Graphics::CreateDefaultRenderPass(const DefaultRenderPassDesc& desc) {
    return m_GfxThread->EnqueueFuture([this, desc = desc](){
        return m_GfxBackend->CreateDefaultRenderPass(desc);
    });
}

Future<ExResult<RenderPassHandle>> Graphics::CreateRenderPass(const RenderPassDesc& desc) {
    return m_GfxThread->EnqueueFuture([this, desc = desc](){
        return m_GfxBackend->CreateRenderPass(desc);
    });
}

Future<ExError> Graphics::DestroyRenderPass(const RenderPassHandle& handle) {
    return m_GfxThread->EnqueueFuture([this, handle = handle](){
        return m_GfxBackend->DestroyRenderPass(handle);
    });
}

Future<ExResult<ResourceSetHandle>> Graphics::CreateResourceSet(const ResourceSetDesc& desc) {
    return m_GfxThread->EnqueueFuture([this, desc = desc](){
        return m_GfxBackend->CreateResourceSet(desc);
    });
}

Future<ExError> Graphics::UpdateResourceSet(const ResourceSetHandle& handle, std::vector<Binding> bindings) {
    return m_GfxThread->EnqueueFuture([this, handle = handle, bindings = bindings](){
        return m_GfxBackend->UpdateResourceSet(handle, bindings);
    });
}

Future<ExError> Graphics::DestroyResourceSet(const ResourceSetHandle& handle) {
    return m_GfxThread->EnqueueFuture([this, handle = handle](){
        return m_GfxBackend->DestroyResourceSet(handle);
    });
}

Future<utils::ExResult<uint32_t>> Graphics::AcquireNextImage() {
    return m_GfxThread->EnqueueFuture([this](){
        return m_GfxBackend->AcquireNextImage();
    });
}

Future<utils::ExResult<FramebufferHandle>> Graphics::GetSwapchainFramebuffer(uint32_t imageIndex) {
    return m_GfxThread->EnqueueFuture([this, imageIndex = imageIndex](){
        return m_GfxBackend->GetSwapchainFramebuffer(imageIndex);
    });
}

Future<utils::ExError> Graphics::Present(uint32_t imageIndex) {
    return m_GfxThread->EnqueueFuture([this, imageIndex = imageIndex](){
        return m_GfxBackend->Present(imageIndex);
    });
}

SharedPtr<ICommandList> Graphics::PrepCommandList() {
    switch (m_GfxType) {
        case GfxType::GL330:
#ifdef __AX_GRAPHICS_GL__
            return std::make_shared<GLCommandList>();
#endif
        default: return SharedPtr<ICommandList>{nullptr}; // TODO
    }
}

Future<ExError> Graphics::Submit(SharedPtr<ICommandList> cmdList) {
    return m_GfxThread->EnqueueFuture([this, cmdList = cmdList](){
        return m_GfxBackend->Execute(*cmdList);
    });
}

const GraphicsCaps& Graphics::Capabilities() {
    std::lock_guard<std::mutex> lock(m_GfxCapsMutex);
    return m_GfxBackend->GetCaps();
}

}