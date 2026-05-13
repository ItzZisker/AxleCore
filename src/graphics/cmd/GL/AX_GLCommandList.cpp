#ifdef __AX_GRAPHICS_GL__

#include "axle/graphics/cmd/GL/AX_GLCommandList.hpp"

#include <mutex>
#include <bit>

namespace axle::gfx {

GLCommandGuard::GLCommandGuard(std::mutex& cmdListMutex, SharedPtr<data::BufferDataStream> commandBuffer) 
    : m_CmdListMutex(cmdListMutex), m_Buffer(commandBuffer) {
    m_CmdListMutex.lock();
}

GLCommandGuard::~GLCommandGuard() {
    m_CmdListMutex.unlock();
}

GLCommandList::GLCommandList() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_CommandBuffer = std::make_shared<data::BufferDataStream>(512);
    m_CommandBuffer->Open().ThrowIfValid();
}

utils::ExError GLCommandList::Begin() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    AX_PROPAGATE_ERROR(m_CommandBuffer->SeekWrite(0));
    AX_PROPAGATE_RESULT_ERROR(m_CommandBuffer->Write((uint8_t)0, m_CommandBuffer->GetLength()));
    AX_PROPAGATE_ERROR(m_CommandBuffer->SeekWrite(0));
    AX_PROPAGATE_RESULT_ERROR(m_CommandBuffer->Write(&CMDL_BEGIN, 2));
    return utils::ExError::NoError();
}

utils::ExError GLCommandList::End() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    AX_PROPAGATE_ERROR(m_CommandBuffer->SeekRead(0));
    AX_PROPAGATE_RESULT_ERROR(m_CommandBuffer->Write(&CMDL_END, 2));
    return utils::ExError::NoError();
}

template<typename CommandStruct>
utils::ExError RecordCommand(std::mutex& mutex, SharedPtr<data::BufferDataStream> commandBuffer, CommandType type, const CommandStruct& strc) {
    std::lock_guard<std::mutex> lock(mutex);
    AX_PROPAGATE_RESULT_ERROR(commandBuffer->Write(&CMD_HEADER, 2));
    AX_PROPAGATE_RESULT_ERROR(commandBuffer->Write(&type, 4));
    AX_PROPAGATE_RESULT_ERROR(commandBuffer->Write(&strc, sizeof(strc)));
    AX_PROPAGATE_RESULT_ERROR(commandBuffer->Write(&CMD_FOOTER, 2));
    return utils::ExError::NoError();
}

utils::ExError GLCommandList::SetViewport(const CommandSetViewport& cmd) {
    return RecordCommand(m_Mutex, m_CommandBuffer, CommandType::SetViewport, cmd);
}

utils::ExError GLCommandList::SetScissor(const CommandSetScissor& cmd) {
    return RecordCommand(m_Mutex, m_CommandBuffer, CommandType::SetScissor, cmd);
}

utils::ExError GLCommandList::BeginRenderPass(const CommandBeginRenderPass& cmd) {
    return RecordCommand(m_Mutex, m_CommandBuffer, CommandType::BeginRenderPass, cmd);
}

utils::ExError GLCommandList::EndRenderPass(const CommandEndRenderPass& cmd) {
    return RecordCommand(m_Mutex, m_CommandBuffer, CommandType::EndRenderPass, cmd);
}

utils::ExError GLCommandList::BindRenderPipeline(const CommandBindRenderPipeline& cmd) {
    return RecordCommand(m_Mutex, m_CommandBuffer, CommandType::BindRenderPipeline, cmd);
}

utils::ExError GLCommandList::BindComputePipeline(const CommandBindComputePipeline& cmd) {
    return RecordCommand(m_Mutex, m_CommandBuffer, CommandType::BindComputePipeline, cmd);
}

utils::ExError GLCommandList::BindVertexBuffer(const CommandBindVertexBuffer& cmd) {
    return RecordCommand(m_Mutex, m_CommandBuffer, CommandType::BindVertexBuffer, cmd);
}

utils::ExError GLCommandList::BindIndexBuffer(const CommandBindIndexBuffer& cmd) {
    return RecordCommand(m_Mutex, m_CommandBuffer, CommandType::BindIndexBuffer, cmd);
}

utils::ExError GLCommandList::BindIndirectBuffer(const CommandBindIndirectBuffer& cmd) {
    return RecordCommand(m_Mutex, m_CommandBuffer, CommandType::BindIndirectBuffer, cmd);
}

utils::ExError GLCommandList::BindResourceSet(const CommandBindResourceSet& cmd) {
    return RecordCommand(m_Mutex, m_CommandBuffer, CommandType::BindResourceSet, cmd);
}

utils::ExError GLCommandList::Draw(const CommandDraw& cmd) {
    return RecordCommand(m_Mutex, m_CommandBuffer, CommandType::Draw, cmd);
}

utils::ExError GLCommandList::DrawInstanced(const CommandDrawInstanced& cmd) {
    return RecordCommand(m_Mutex, m_CommandBuffer, CommandType::DrawInstanced, cmd);
}

utils::ExError GLCommandList::DrawIndexed(const CommandDrawIndexed& cmd) {
    return RecordCommand(m_Mutex, m_CommandBuffer, CommandType::DrawIndexed, cmd);
}

utils::ExError GLCommandList::DrawIndexedInstanced(const CommandDrawIndexedInstanced& cmd) {
    return RecordCommand(m_Mutex, m_CommandBuffer, CommandType::DrawIndexedInstanced, cmd);
}

utils::ExError GLCommandList::DrawIndirect(const CommandDrawIndirect& cmd) {
    return RecordCommand(m_Mutex, m_CommandBuffer, CommandType::DrawIndirect, cmd);
}

utils::ExError GLCommandList::DrawIndirectIndexed(const CommandDrawIndirectIndexed& cmd) {
    return RecordCommand(m_Mutex, m_CommandBuffer, CommandType::DrawIndirectIndexed, cmd);
}

}
#endif