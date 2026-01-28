#pragma once

#ifdef __AX_GRAPHICS_GL__
#include "axle/graphics/cmd/AX_GraphicsCommand.hpp"

#include "axle/core/AX_GameLoop.hpp"
#include "axle/core/ctx/AX_IRenderContext.hpp"
#include "axle/core/concurrency/AX_TaskQueue.hpp"

#include "axle/utils/AX_Types.hpp"

#include <cmath>

namespace axle::gs {

void GL_ValidateCmdPool(const GS_CommandPool& pool);
void GL_ValidateContext(SharedPtr<core::RenderThreadContext> ctx);

class GS_GLICmd;

class GS_GLCommandPoolHandle {
private:
    SharedPtr<core::RenderThreadContext> m_Ctx;
    core::TaskQueue m_Queue;
public:
    template <typename T_Cmd>
    requires std::is_base_of_v<GS_GLICmd, T_Cmd>
    void EnqueueCommand(T_Cmd cmd) {
        m_Queue.Enqueue([this, cmd = std::move(cmd)](){
            cmd.Dispatch(m_Ctx);
        });
    }

    void DispatchAndFlush();
};

class GS_GLICmd : public GS_CommandNode {
public:
    GS_GLICmd() : GS_CommandNode(true) {}
    void Queue(const GS_CommandPool& pool) override;
};

class GS_GLCommandPool : GS_CommandPool {
private:
    UniquePtr<GS_GLCommandPoolHandle> m_Handle{nullptr};
public:
    void Init(SharedPtr<core::RenderThreadContext> ctx) override;
    void Dispatch(SharedPtr<core::RenderThreadContext> ctx) override;

    core::GraphicsBackend GetRendererEnum() const override { return core::Graphics_GL330; }
    GS_Handle GetHandle() const override { return GS_CreateHVal(GSH_Internal, (size_t)m_Handle.get()); }
};

class GS_GLCmdClear : public GS_GLICmd {
private:
    int32_t m_buffTypes;
public:
    explicit GS_GLCmdClear(int32_t buffTypes) : m_buffTypes(buffTypes) {}

    GS_Handle Dispatch(SharedPtr<core::RenderThreadContext> ctx) const override;
};

class GS_GLCmdSetView : public GS_GLICmd {
private:
    int32_t m_x0, m_y0, m_Width, m_Height;
public:
    explicit GS_GLCmdSetView(int32_t x0, int32_t y0, int32_t width, int32_t height)
        : m_x0(x0), m_y0(y0), m_Width(width), m_Height(height) {}

    GS_Handle Dispatch(SharedPtr<core::RenderThreadContext> ctx) const override;
};

class GS_GLCmdSetColor : public GS_GLICmd {
private:
    float_t m_r, m_g, m_b, m_a;
public:
    explicit GS_GLCmdSetColor(float_t r, float_t g, float_t b, float_t a)
        : m_r(r), m_g(g), m_b(b), m_a(a) {}

    GS_Handle Dispatch(SharedPtr<core::RenderThreadContext> ctx) const override;
};

class GS_GLCmdSetStencil : public GS_GLICmd {
private:
    int32_t m_Stencil;
public:
    explicit GS_GLCmdSetStencil(int32_t stencil)
        : m_Stencil(stencil) {}

    GS_Handle Dispatch(SharedPtr<core::RenderThreadContext> ctx) const override;
};

class GS_GLCmdSetDepth : public GS_GLICmd {
private:
    double_t m_Depth;
public:
    explicit GS_GLCmdSetDepth(double_t depth)
        : m_Depth(depth) {}

    GS_Handle Dispatch(SharedPtr<core::RenderThreadContext> ctx) const override;
};

}
#endif