#pragma once

#include "AX_GraphicsTypes.hpp"

#include "axle/core/AX_GameLoop.hpp"
#include "axle/utils/AX_Types.hpp"

#include <cstdint>

#define AX_HEX_RGB(hex) \
    ((float)((hex >> 16) & 0xFF) / 255.0f), \
    ((float)((hex >> 8)  & 0xFF) / 255.0f), \
    ((float)((hex)       & 0xFF) / 255.0f), \
    1.0f

#define AX_HEX_RGBA(hex) \
    ((float)((hex >> 24) & 0xFF) / 255.0f), \
    ((float)((hex >> 16) & 0xFF) / 255.0f), \
    ((float)((hex >> 8)  & 0xFF) / 255.0f), \
    ((float)((hex)       & 0xFF) / 255.0f)

#define AX_HEX_RGB_A(hex, alpha) \
    ((float)((hex >> 16) & 0xFF) / 255.0f), \
    ((float)((hex >> 8)  & 0xFF) / 255.0f), \
    ((float)((hex)       & 0xFF) / 255.0f), \
    (alpha)

namespace axle::gfx {

class CommandBuffer;
class CommandList {
public:
    CommandList();

    void BeginRenderPass(RenderPassHandle);
    void BindPipeline(PipelineHandle);
    void BindVertexBuffer(BufferHandle, uint32_t slot);
    void Draw(uint32_t vertexCount);
    void EndRenderPass();
private:
    UniquePtr<CommandBuffer> m_Buffer{nullptr}; // POD command stream
};

class Graphics {
private:
    SharedPtr<core::ThreadContextGfx> m_GfxThread;
public:
    Graphics(SharedPtr<core::ThreadContextGfx> gfxThread) : m_GfxThread(gfxThread) {}
    ~Graphics();

    // Resource creation
    BufferHandle   CreateBuffer(BufferDesc);
    TextureHandle  CreateTexture(TextureDesc);
    ShaderHandle   CreateShader(ShaderDesc);
    PipelineHandle CreatePipeline(PipelineDesc);
    RenderPassHandle CreateRenderPass(RenderPassDesc);

    // Frame
    void BeginFrame();
    void EndFrame();

    // RenderContext Options
    void SetVSync(bool b);

    // Command recording
    CommandList BeginCommandList();
    void Submit(CommandList&& cmdList);

    // Capabilities
    const GraphicsCaps& Capabilities() const;
};


}