#include "axle/graphics/cmd/GL/AX_GLCommandList.hpp"

namespace axle::gfx {

GLCommandList::GLCommandList() {}

void GLCommandList::Begin() {
    m_Commands.clear();
}

void GLCommandList::End() {}

void GLCommandList::BeginRenderPass(RenderPassHandle pass) {
    m_Commands.push_back({ GLCommandType::BeginRenderPass, { pass.index, pass.generation } });
}

void GLCommandList::EndRenderPass() {
    m_Commands.push_back({ GLCommandType::EndRenderPass, {} });
}

void GLCommandList::BindPipeline(PipelineHandle pipeline) {
    m_Commands.push_back({ GLCommandType::BindPipeline, { pipeline.index, pipeline.generation } });
}

void GLCommandList::BindVertexBuffer(BufferHandle buffer) {
    m_Commands.push_back({ GLCommandType::BindVertexBuffer, { buffer.index, buffer.generation } });
}

void GLCommandList::BindIndexBuffer(BufferHandle buffer) {
    m_Commands.push_back({ GLCommandType::BindIndexBuffer, { buffer.index, buffer.generation } });
}

void GLCommandList::Draw(uint32_t vertexCount, uint32_t firstVertex) {
    m_Commands.push_back({ GLCommandType::Draw, { vertexCount, firstVertex } });
}

void GLCommandList::DrawIndexed(uint32_t indexCount, uint32_t firstIndex) {
    m_Commands.push_back({ GLCommandType::DrawIndexed, { indexCount, firstIndex } });
}

void GLCommandList::DrawIndirect(BufferHandle indirect, uint32_t offset) {
    m_Commands.push_back({ GLCommandType::DrawIndirect, { indirect.index, indirect.generation, offset } });
}

}
