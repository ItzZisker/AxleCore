#pragma once

#include "axle/graphics/AX_Graphics.hpp"

#include <vector>

namespace axle::gfx {

enum class GLCommandType {
    BeginRenderPass,
    EndRenderPass,
    BindPipeline,
    BindBuffer,
    Draw,
    DrawIndexed,
    DrawIndirect,
    DrawIndirectIndexed,
};

struct GLCommand {
    GLCommandType type;
    std::vector<size_t> args; // generic payload
};

class GLCommandList final : public ICommandList {
public:
    GLCommandList();

    void Begin() override;
    void End() override;

    void BeginRenderPass(
        const RenderPassHandle& pass,
        const FramebufferHandle& framebuffer,
        const RenderPassClear& clear = {}
    ) override;
    void EndRenderPass() override;

    void BindRenderPipeline(const RenderPipelineHandle& pipeline) override;
    void BindComputePipeline(const ComputePipelineHandle& pipeline) override;
    void BindVertexBuffer(const BufferHandle& buffer) override;
    void BindIndexBuffer(const BufferHandle& buffer) override;

    void Draw(uint32_t vertexCount, uint32_t firstVertex) override;
    void DrawIndexed(uint32_t indexCount, uint32_t firstIndex) override;
    void DrawIndirect(const BufferHandle& indirectBuff, uint32_t offset) override;

    const std::vector<GLCommand>& Commands() const {
        return m_Commands;
    }
private:
    std::vector<GLCommand> m_Commands;
};

} // namespace axle::gfx
