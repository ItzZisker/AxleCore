#pragma once

#include "axle/graphics/AX_Graphics.hpp"
#include <vector>

namespace axle::gfx {

enum class GLCommandType {
    BeginRenderPass,
    EndRenderPass,
    BindPipeline,
    BindVertexBuffer,
    BindIndexBuffer,
    Draw,
    DrawIndexed,
    DrawIndirect
};

struct GLCommand {
    GLCommandType type;
    size_t args[4]; // generic payload
};

class GLCommandList final : public ICommandList {
public:
    GLCommandList();

    void Begin() override;
    void End() override;

    void BeginRenderPass(RenderPassHandle pass) override;
    void EndRenderPass() override;

    void BindPipeline(PipelineHandle pipeline) override;
    void BindVertexBuffer(BufferHandle buffer) override;
    void BindIndexBuffer(BufferHandle buffer) override;

    void Draw(uint32_t vertexCount, uint32_t firstVertex) override;
    void DrawIndexed(uint32_t indexCount, uint32_t firstIndex) override;
    void DrawIndirect(BufferHandle indirect, uint32_t offset) override;

    const std::vector<GLCommand>& Commands() const { return m_Commands; }
private:
    std::vector<GLCommand> m_Commands;
};

} // namespace axle::gfx
