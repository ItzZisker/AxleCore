#pragma once

#include "axle/graphics/AX_Graphics.hpp"

#include <vector>

namespace axle::gfx {

enum class GLCommandType {
    BeginRenderPass,
    EndRenderPass,
    BindRenderPipeline,
    BindComputePipeline,
    BindVertexBuffer,
    BindIndexBuffer,
    BindIndirectBuffer,
    BindResourceSet,
    Draw,
    DrawInstanced,
    DrawIndexed,
    DrawIndexedInstanced,
    DrawIndirect,
    DrawIndirectIndexed
};

struct GLCommand {
    GLCommandType type;
    std::vector<std::size_t> args; // generic payload
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
    void BindIndirectBuffer(const BufferHandle& buffer) override;

    void BindResourceSet(const ResourceSetHandle& res) override;

    void Draw(
        uint32_t vertexCount,
        uint32_t firstVertex = 0
    ) override;

    void DrawInstanced(
        uint32_t vertexCount,
        uint32_t instanceCount,
        uint32_t firstVertex = 0
    ) override;

    void DrawIndexed(
        uint32_t indexCount,
        uint32_t firstIndex = 0
    ) override;

    void DrawIndexedInstanced(
        uint32_t indexCount,
        uint32_t instanceCount,
        uint32_t firstIndex = 0
    ) override;

    void DrawIndirect(
        uint32_t offset,
        uint32_t count,
        uint32_t stride
    ) override;

    void DrawIndirectIndexed(
        uint32_t offset,
        uint32_t count,
        uint32_t stride
    ) override;

    const std::vector<GLCommand>& Commands() const {
        return m_Commands;
    }
private:
    std::vector<GLCommand> m_Commands;
};

} // namespace axle::gfx
