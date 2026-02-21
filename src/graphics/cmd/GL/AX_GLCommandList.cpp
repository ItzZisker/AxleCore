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
    m_Commands.push_back({GLCommandType::BeginRenderPass, {
        pass.index,
        pass.generation,
        framebuffer.index,
        framebuffer.generation,
        (size_t)clear.clearColor[0],
        (size_t)clear.clearColor[1],
        (size_t)clear.clearColor[2],
        (size_t)clear.clearDepth,
        (size_t)clear.clearStencil
    }});
}

void GLCommandList::EndRenderPass() {
    m_Commands.push_back({GLCommandType::EndRenderPass, {} });
}

void GLCommandList::BindRenderPipeline(const RenderPipelineHandle& pipeline) {
    m_Commands.push_back({GLCommandType::BindRenderPipeline, {
        pipeline.index,
        pipeline.generation
    }});
}

void GLCommandList::BindComputePipeline(const ComputePipelineHandle& pipeline) {
    m_Commands.push_back({GLCommandType::BindComputePipeline, {
        pipeline.index,
        pipeline.generation
    }});
}

void GLCommandList::BindVertexBuffer(const BufferHandle& buffer) {
    m_Commands.push_back({GLCommandType::BindVertexBuffer, {
        buffer.index,
        buffer.generation
    }});
}

void GLCommandList::BindIndexBuffer(const BufferHandle& buffer) {
    m_Commands.push_back({GLCommandType::BindIndexBuffer, {
        buffer.index,
        buffer.generation
    }});
}

void GLCommandList::BindIndirectBuffer(const BufferHandle& buffer) {
    m_Commands.push_back({GLCommandType::BindIndirectBuffer, {
        buffer.index,
        buffer.generation
    }});
}

void GLCommandList::BindResourceSet(const ResourceSetHandle& res) {
    m_Commands.push_back({GLCommandType::BindResourceSet, {
        res.index,
        res.generation
    }});
}

void GLCommandList::Draw(
    uint32_t vertexCount,
    uint32_t firstVertex
) {
    m_Commands.push_back({GLCommandType::Draw, {
        vertexCount,
        firstVertex
    }});
}

void GLCommandList::DrawInstanced(
    uint32_t vertexCount,
    uint32_t instanceCount,
    uint32_t firstVertex
) {
    m_Commands.push_back({GLCommandType::DrawInstanced, {
        vertexCount,
        instanceCount,
        firstVertex
    }});
}

void GLCommandList::DrawIndexed(
    uint32_t indexCount,
    uint32_t firstIndex
) {
    m_Commands.push_back({GLCommandType::DrawIndexed, {
        indexCount,
        firstIndex
    }});
}

void GLCommandList::DrawIndexedInstanced(
    uint32_t indexCount,
    uint32_t instanceCount,
    uint32_t firstIndex
) {
    m_Commands.push_back({GLCommandType::DrawIndexedInstanced, {
        indexCount,
        instanceCount,
        firstIndex
    }});
}

void GLCommandList::DrawIndirect(
    uint32_t offset,
    uint32_t count,
    uint32_t stride
) {
    m_Commands.push_back({GLCommandType::DrawIndirect, {
        offset,
        count,
        stride
    }});
}

void GLCommandList::DrawIndirectIndexed(
    uint32_t offset,
    uint32_t count,
    uint32_t stride
) {
    m_Commands.push_back({GLCommandType::DrawIndirectIndexed, {
        offset,
        count,
        stride
    }});
}

}
#endif