#ifdef __AX_GRAPHICS_GL__

#include "axle/graphics/cmd/GL/AX_GLCommandList.hpp"
#include "axle/graphics/AX_Graphics.hpp"

namespace axle::gfx {

// TODO: Verify arguments

GLCommandList::GLCommandList() {}

void GLCommandList::Begin() {
    m_Commands.clear();
}

void GLCommandList::End() {}

void GLCommandList::BeginRenderPass(
    const RenderPassHandle& pass,
    const FramebufferHandle& framebuffer,
    const RenderPassClear& clear
) {
    m_Commands.push_back({ GLCommandType::BeginRenderPass, {
        pass.index,
        pass.generation,
        framebuffer.index,
        framebuffer.generation,
        (size_t)clear.clearColor[0],
        (size_t)clear.clearColor[1],
        (size_t)clear.clearColor[2],
        (size_t)clear.clearDepth,
        (size_t)clear.clearStencil
    } });
}

void GLCommandList::EndRenderPass() {
    m_Commands.push_back({ GLCommandType::EndRenderPass, {} });
}

void GLCommandList::BindRenderPipeline(const RenderPipelineHandle& pipeline) {
    m_Commands.push_back({ GLCommandType::BindPipeline, { pipeline.index, pipeline.generation } });
}

void GLCommandList::BindVertexBuffer(const BufferHandle& buffer) {
    m_Commands.push_back({ GLCommandType::BindBuffer, { buffer.index, buffer.generation } });
}

void GLCommandList::BindIndexBuffer(const BufferHandle& buffer) {
    m_Commands.push_back({ GLCommandType::BindBuffer, { buffer.index, buffer.generation } });
}

void GLCommandList::Draw(uint32_t vertexCount, uint32_t firstVertex) {
    m_Commands.push_back({ GLCommandType::Draw, { vertexCount, firstVertex } });
}

void GLCommandList::DrawIndexed(uint32_t indexCount, uint32_t firstIndex) {
    m_Commands.push_back({ GLCommandType::DrawIndexed, { indexCount, firstIndex } });
}

void GLCommandList::DrawIndirect(const BufferHandle& indirectBuff, uint32_t offset) {
    m_Commands.push_back({ GLCommandType::DrawIndirect, { indirectBuff.index, indirectBuff.generation, offset } });
}

}
#endif