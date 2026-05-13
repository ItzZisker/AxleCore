#pragma once

#include "axle/graphics/cmd/AX_ICommandList.hpp"

#include "axle/data/AX_DataStreamImplBuffer.hpp"

#include "axle/utils/AX_Expected.hpp"

#include <vector>
#include <mutex>

namespace axle::gfx {

const uint16_t CMDL_BEGIN = 1111;

const uint16_t CMD_HEADER = 1441;
const uint16_t CMD_FOOTER = 1771;

const uint16_t CMDL_END = 1991;

class GLCommandGuard {
private:
    std::mutex& m_CmdListMutex;
public:
    const SharedPtr<data::BufferDataStream> m_Buffer{nullptr};

    explicit GLCommandGuard(std::mutex& cmdListMutex, SharedPtr<data::BufferDataStream> commandBuffer);
    ~GLCommandGuard();

    GLCommandGuard(const GLCommandGuard&) = delete;
    GLCommandGuard& operator=(const GLCommandGuard&) = delete;

    GLCommandGuard(GLCommandGuard&&) = delete;
    GLCommandGuard& operator=(GLCommandGuard&&) = delete;
};

class GLCommandList final : public ICommandList {
private:
    SharedPtr<data::BufferDataStream> m_CommandBuffer{nullptr};
    std::mutex m_Mutex;
public:
    GLCommandList();

    utils::ExError Begin() override;
    utils::ExError End() override;

    utils::ExError SetViewport(const CommandSetViewport&) override;
    utils::ExError SetScissor(const CommandSetScissor&) override;

    utils::ExError BeginRenderPass(const CommandBeginRenderPass&) override;
    utils::ExError EndRenderPass(const CommandEndRenderPass&) override;

    utils::ExError BindRenderPipeline(const CommandBindRenderPipeline&) override;
    utils::ExError BindComputePipeline(const CommandBindComputePipeline&) override;

    utils::ExError BindVertexBuffer(const CommandBindVertexBuffer&) override;
    utils::ExError BindIndexBuffer(const CommandBindIndexBuffer&) override;
    utils::ExError BindIndirectBuffer(const CommandBindIndirectBuffer&) override;
    utils::ExError BindResourceSet(const CommandBindResourceSet&) override;

    utils::ExError Draw(const CommandDraw&) override;
    utils::ExError DrawInstanced(const CommandDrawInstanced&) override;
    utils::ExError DrawIndexed(const CommandDrawIndexed&) override;
    utils::ExError DrawIndexedInstanced(const CommandDrawIndexedInstanced&) override;
    utils::ExError DrawIndirect(const CommandDrawIndirect&) override;
    utils::ExError DrawIndirectIndexed(const CommandDrawIndirectIndexed&) override;
protected:
    GLCommandGuard CommandGuard() { return GLCommandGuard(m_Mutex, m_CommandBuffer); }

    friend class GLGraphicsBackend;
};

} // namespace axle::gfx
