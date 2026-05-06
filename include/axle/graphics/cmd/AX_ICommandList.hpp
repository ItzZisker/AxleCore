#pragma once

#include "axle/graphics/AX_GraphicsParams.hpp"

#include "axle/utils/AX_Expected.hpp"

#include <cstdint>
#include <variant>


namespace axle::core {
    class ThreadContextGfx;   
}

namespace axle::gfx {

enum class CommandType : uint32_t {
    SetViewport,
    SetScissor,

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

struct CommandSetViewport {
    float x;
    float y;
    float width;
    float height;
    float minDepth = 0.0f;
    float maxDepth = 1.0f;
};

struct CommandSetScissor {
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
};

struct CommandBeginRenderPass {
    RenderPassHandle pass;
    FramebufferHandle framebuffer;
    RenderPassClear clear = {};
};

struct CommandEndRenderPass {
    RenderPassHandle pass;
};

struct CommandBindRenderPipeline {
    RenderPipelineHandle handle;
};

struct CommandBindComputePipeline {
    ComputePipelineHandle handle;
};

struct CommandBindVertexBuffer {
    BufferHandle handle;
};

struct CommandBindIndexBuffer {
    BufferHandle handle;
};

struct CommandBindIndirectBuffer {
    BufferHandle handle;
};

struct CommandBindResourceSet {
    ResourceSetHandle handle;
};

struct CommandDraw {
    uint32_t vertexCount;
    uint32_t firstVertex = 0;
};

struct CommandDrawInstanced {
    uint32_t vertexCount;
    uint32_t instanceCount;
    uint32_t firstVertex = 0;
};

struct CommandDrawIndexed {
    uint32_t indexCount;
    uint32_t firstIndex = 0;
};

struct CommandDrawIndexedInstanced {
    uint32_t indexCount;
    uint32_t instanceCount;
    uint32_t firstIndex = 0;
};

struct CommandDrawIndirect {
    uint32_t offset;
    uint32_t count;
    uint32_t stride;
};

struct CommandDrawIndirectIndexed {
    uint32_t firstIndex;
    uint32_t count;
    uint32_t stride;
};

class ICommandList {
public:
    virtual ~ICommandList() = default;

    Future<utils::ExError> Submit(SharedPtr<core::ThreadContextGfx> gfxThread);

    static SharedPtr<gfx::ICommandList> Create(SharedPtr<core::ThreadContextGfx> gfxThread);

    virtual utils::ExError Begin() = 0;
    virtual utils::ExError End() = 0;

    virtual utils::ExError SetViewport(const CommandSetViewport&) = 0;
    virtual utils::ExError SetScissor(const CommandSetScissor&) = 0;

    virtual utils::ExError BeginRenderPass(const CommandBeginRenderPass&) = 0;
    virtual utils::ExError EndRenderPass(const CommandEndRenderPass&) = 0;

    virtual utils::ExError BindRenderPipeline(const CommandBindRenderPipeline&) = 0;
    virtual utils::ExError BindComputePipeline(const CommandBindComputePipeline&) = 0;

    virtual utils::ExError BindVertexBuffer(const CommandBindVertexBuffer&) = 0;
    virtual utils::ExError BindIndexBuffer(const CommandBindIndexBuffer&) = 0;
    virtual utils::ExError BindIndirectBuffer(const CommandBindIndirectBuffer&) = 0;
    virtual utils::ExError BindResourceSet(const CommandBindResourceSet&) = 0;

    virtual utils::ExError Draw(const CommandDraw&) = 0;
    virtual utils::ExError DrawInstanced(const CommandDrawInstanced&) = 0;
    virtual utils::ExError DrawIndexed(const CommandDrawIndexed&) = 0;
    virtual utils::ExError DrawIndexedInstanced(const CommandDrawIndexedInstanced&) = 0;
    virtual utils::ExError DrawIndirect(const CommandDrawIndirect&) = 0;
    virtual utils::ExError DrawIndirectIndexed(const CommandDrawIndirectIndexed&) = 0;
};

}