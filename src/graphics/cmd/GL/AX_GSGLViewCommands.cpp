#ifdef __AX_GRAPHICS_GL__
#include "axle/graphics/cmd/GL/AX_GSGLViewCommands.hpp"
#include "axle/graphics/cmd/AX_GraphicsCommand.hpp"

#include "axle/utils/AX_Types.hpp"

#include <stdexcept>
#include <memory>

namespace axle::graphics {

void GL_ValidateCmdPool(const GS_CommandPool& pool) {
    if (pool.GetRendererEnum() != core::Graphics_GL330)
        throw std::runtime_error("AX Exception: GS_GLICmd()->Queue(): Invalid CommandPool");
}

void GL_ValidateContext(SharedPtr<core::RenderThreadContext> ctx) {
    auto glctx = ctx->GetContext();
    if (!glctx || glctx->GetType() != core::Graphics_GL330) {
        throw std::runtime_error("AX Exception: Invalid Context, either not available or not a valid 'GL330' Context");
    }
}

void GS_GLCommandPoolHandle::DispatchAndFlush() {
    auto cmds = m_Queue.MoveJobs();
    for (auto& cmd : cmds) cmd();
}

void GS_GLICmd::Queue(const GS_CommandPool& pool) {
    GL_ValidateCmdPool(pool);    
    auto handle = GS_ReadHPtr<GS_GLCommandPoolHandle>(pool.GetHandle());
    handle->EnqueueCommand(*this);
}

void GS_GLCommandPool::Init(SharedPtr<core::RenderThreadContext> ctx) {
    GL_ValidateContext(ctx);
    m_Handle = std::make_unique<GS_GLCommandPoolHandle>();
}

void GS_GLCommandPool::Dispatch(SharedPtr<core::RenderThreadContext> ctx) {
    GL_ValidateContext(ctx);
    m_Handle->DispatchAndFlush();
}

GS_Handle GS_GLCmdClear::Dispatch(SharedPtr<core::RenderThreadContext> ctx) const {
    GL_ValidateContext(ctx);
    int32_t _glBuffTypes = 0;
    if (m_buffTypes & GS_BufferType::GSB_Stencil) _glBuffTypes |= GL_STENCIL_BUFFER_BIT;
    if (m_buffTypes & GS_BufferType::GSB_Color)   _glBuffTypes |= GL_COLOR_BUFFER_BIT;
    if (m_buffTypes & GS_BufferType::GSB_Depth)   _glBuffTypes |= GL_DEPTH_BUFFER_BIT;
    glClear(_glBuffTypes);
    return GS_CreateHValNull();
}

GS_Handle GS_GLCmdSetView::Dispatch(SharedPtr<core::RenderThreadContext> ctx) const  {
    GL_ValidateContext(ctx);
    glViewport(m_x0, m_y0, m_Width, m_Height);
    return GS_CreateHValNull();
}

GS_Handle GS_GLCmdSetColor::Dispatch(SharedPtr<core::RenderThreadContext> ctx) const {
    GL_ValidateContext(ctx);
    glClearColor(m_r, m_g, m_b, m_a);
    return GS_CreateHValNull();
}

GS_Handle GS_GLCmdSetStencil::Dispatch(SharedPtr<core::RenderThreadContext> ctx) const {
    GL_ValidateContext(ctx);
    glClearStencil(m_Stencil);
    return GS_CreateHValNull();
}

GS_Handle GS_GLCmdSetDepth::Dispatch(SharedPtr<core::RenderThreadContext> ctx) const {
    GL_ValidateContext(ctx);
    glClearDepth(m_Depth);
    return GS_CreateHValNull();
}

}
#endif