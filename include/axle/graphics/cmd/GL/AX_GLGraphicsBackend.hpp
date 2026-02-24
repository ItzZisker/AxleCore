#pragma once

#ifdef __AX_GRAPHICS_GL__

#include "AX_GLCommandList.hpp"

#include "axle/graphics/AX_Graphics.hpp"
#include "axle/utils/AX_Expected.hpp"

#include <slang.h>
#include <slang-com-ptr.h>

#include <glad/gl.h>

#include <type_traits>
#include <vector>

#ifdef AX_DEBUG
#include <iostream>

namespace axle::gfx {
    const char* GLErrorToString(GLenum err) {
        switch (err) {
            case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
            case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
            case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
            case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
            case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
            default: return "UNKNOWN_ERROR";
        }
    }
}

#define GL_CALL(x)                                    \
    do {                                              \
        while (glGetError() != GL_NO_ERROR);          \
        x;                                            \
        GLenum err;                                   \
        while ((err = glGetError()) != GL_NO_ERROR) { \
            std::cerr << "GL Error: "                 \
                    << err                            \
                    << ": "                           \
                    << GLErrorToString(err)           \
                    << " in " << #x                   \
                    << " at " << __FILE__             \
                    << ":" << __LINE__                \
                    << std::endl;                     \
        }                                             \
    } while (0)
#else
#define GL_CALL(x) x
#endif

#ifndef GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT 0x8C4D
#endif

#ifndef GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT 0x8C4F
#endif

namespace axle::gfx {

GLenum ToGLTarget(TextureType type);
GLenum ToGLInternalFormat(TextureFormat fmt);
GLenum ToGLFormat(TextureFormat fmt);
GLenum ToGLType(TextureFormat fmt);
GLenum ToGLWrap(TextureWrap wrap);
GLenum ToGLFilter(TextureFilter filter);
GLenum ToGLStage(ShaderStage stg);
GLenum ToGLType(VertexAttributeType type);
GLenum ToGLCompare(CompareOp cmpop);
GLenum ToGLBlendFactor(BlendFactor bFactor);
GLenum ToGLBlendOp(BlendOp bOp);
GLenum ToGLStencilOp(StencilOp sOp);
GLenum ToGLBarrierBit(ResourceState state);
GLenum ToGLPolyMode(PolyMode polyMode);

class GLContextGuard {
private:
    SharedPtr<core::IRenderContext> m_RCtx;
public:
    GLContextGuard(SharedPtr<core::IRenderContext> ctx); // Make Current
    ~GLContextGuard();                                   // Release
};

template <typename H>
struct GLCommandBinding {
    H handle{};
    bool bound{false};

    void Bind(const H& h) {
        handle = h;
        bound = true;
    }

    void UnBind() {
        handle = {};
        bound = false;
    }
};

struct GLInternal {
    uint32_t index{0};
    uint32_t generation{1};
    bool alive{false};
};

struct GLProgram : public GLInternal {
    GLuint id{0};
};

struct GLBuffer : public GLInternal {
    GLuint id{0};
    BufferUsage usage;
    size_t size{0};
    uint32_t slot{0};
};

struct GLRenderPipeline : public GLInternal {
    ShaderHandle program{};
    GLuint vao{0};
    GLuint vbo{0};
    GLuint ebo{0};
    RenderPipelineDesc desc;
    VertexLayout vaoLayout{};
};

struct GLComputePipeline : public GLInternal {
    ShaderHandle program;
    ComputePipelineDesc desc;
};

struct GLTexture : public GLInternal {
    GLuint id{0};
    GLuint resolveId{0};
    TextureDesc desc;
};

struct GLRenderPass : public GLInternal {
    RenderPassDesc desc;
    GLCommandBinding<FramebufferHandle> fbInUse{};
};

struct GLFramebuffer : public GLInternal {
    GLuint fbo{0};
    
    bool hasDepth{false};
    bool hasStencil{false};
    GLuint depthStencilTexture{0};

    uint32_t width{0};
    uint32_t height{0};

    bool hasResolve{false};
    GLuint resolveFbo{0};

    RenderPassHandle renderPass;

    bool isSwapchain{false};
};

struct GLSwapchain {
    uint32_t width{0};
    uint32_t height{0};
    TextureFormat format{TextureFormat::RGBA8_UINT};
    FramebufferHandle backbuffer{};
};

struct GLResourceSet : public GLInternal {
    std::vector<Binding> bindings;
    uint32_t layoutID{0};
    uint32_t version{1};
};

struct GLStateCache {
    GLuint program{0};

    bool cullEnabled{false};
    GLenum cullFace{GL_BACK};
    GLenum frontFace{GL_CCW};
    GLenum polygonMode{GL_FILL};

    bool depthTest{false};
    bool depthWrite{true};
    GLenum depthFunc{GL_LESS};

    bool stencilTest{false};

    bool blendEnabled{false};
    GLenum srcColor{GL_ONE};
    GLenum dstColor{GL_ZERO};
    GLenum srcAlpha{GL_ONE};
    GLenum dstAlpha{GL_ZERO};
    GLenum blendColorOp{GL_FUNC_ADD};
    GLenum blendAlphaOp{GL_FUNC_ADD};

    GLuint vao{0};
};

/*
    TODO:
    - Pass IGraphicsBackend to ThreadContextGfx instead of IRenderContext, and use IGraphicsBackend->Present() to Swap buffers, so its backend-managed
    - Make IGraphicsBackend->VSyncToggleSwapchain() to toggle vsync for swapchains by UI event (must be called via main thread, just like ResizeSwapchain())
    - Once done, draw Rainbow RGB scroll and verify if its working or nah.
    - Make ThreadContextAudio for supporting audio stream ticking, decoding and playing sound ques asynch.
    - Remove every piece of unhandled exception thrown in code, specially in DataSerializers and Audio code (mostly Old Codes have that)
    - Implement logger like spdlog, based on previous java codes of MiddleWare Console, (Flushing, Compiled Patterns, ANSI) and log Vulkan/GL errors through that or engine-related stuff
*/

class GLGraphicsBackend final : public IGraphicsBackend {
private:
    SharedPtr<core::IRenderContext> m_Context{nullptr};
    GladGLContext* m_GL{nullptr};

    GraphicsCaps m_Capabilities{};
    bool m_IsCore{false};
public:
    GLGraphicsBackend(SharedPtr<core::IRenderContext> context);
    ~GLGraphicsBackend() override;

    bool IsES() const { return !m_IsCore; }
    bool IsCore() const { return m_IsCore; }

    const GraphicsCaps& GetCaps() const override;
    bool SupportsCap(GraphicsCapEnum cap) override;
    utils::ExResult<GraphicsCaps> QueryCaps() override;

    utils::ExResult<SwapchainHandle> CreateSwapchain(const SwapchainDesc& desc) override;
    utils::AXError DestroySwapchain(const SwapchainHandle& desc) override;
    utils::AXError ResizeSwapchain(const SwapchainHandle& desc, uint32_t width, uint32_t height) override;

    utils::ExResult<BufferHandle> CreateBuffer(const BufferDesc& desc) override;
    utils::AXError UpdateBuffer(const BufferHandle& handle, size_t offset, size_t size, const void* data) override;
    utils::AXError DestroyBuffer(const BufferHandle& handle) override;

    utils::ExResult<TextureHandle> CreateTexture(const TextureDesc& desc) override;
    utils::AXError UpdateTexture(const TextureHandle& handle, const TextureSubDesc& subDesc, const void* data) override;
    utils::AXError DestroyTexture(const TextureHandle& handle) override;

    utils::ExResult<FramebufferHandle> CreateFramebuffer(const FramebufferDesc& handle) override;
    utils::AXError DestroyFramebuffer(const FramebufferHandle& handle) override;

    utils::ExResult<ShaderHandle> CreateProgram(const ShaderDesc& desc) override;
    utils::AXError DestroyProgram(const ShaderHandle& handle) override;

    utils::ExResult<RenderPipelineHandle> CreateRenderPipeline(const RenderPipelineDesc& desc) override;
    utils::AXError DestroyRenderPipeline(const RenderPipelineHandle& handle) override;

    utils::ExResult<ComputePipelineHandle> CreateComputePipeline(const ComputePipelineDesc& desc) override;
    utils::AXError DestroyComputePipeline(const ComputePipelineHandle& handle) override;

    utils::ExResult<RenderPassHandle> CreateRenderPass(const RenderPassDesc& desc) override;
    utils::AXError DestroyRenderPass(const RenderPassHandle& handle) override;

    utils::ExResult<ResourceSetHandle> CreateResourceSet(const ResourceSetDesc& desc) override;
    utils::AXError UpdateResourceSet(const ResourceSetHandle& handle, Span<Binding> bindings) override;
    utils::AXError DestroyResourceSet(const ResourceSetHandle& handle) override;

    utils::AXError Execute(const ICommandList& cmd) override;
    utils::AXError Dispatch(const ICommandList& cmd, uint32_t x, uint32_t y, uint32_t z) override;
    utils::AXError Barrier(const ICommandList& cmd, Span<ResourceTransition> transitions) override;

    utils::ExResult<uint32_t> AcquireNextImage() override;
    utils::ExResult<FramebufferHandle> GetSwapchainFramebuffer(uint32_t imageIndex) override;
    utils::AXError Present(uint32_t imageIndex) override;

    template <typename T>
    requires std::is_base_of_v<GLInternal, T>
    inline T& ReserveHandle(
        std::vector<T>& handles,
        std::vector<uint32_t>& frees
    ) {
        uint32_t index;
        if (!frees.empty()) {
            index = frees.back();
            frees.pop_back();
        } else {
            index = static_cast<uint32_t>(handles.size());
            handles.emplace_back();
            handles.back().index = index;
        }
        return handles[index];
    }

    template <typename TI>
    requires std::is_base_of_v<GLInternal, TI>
    bool IsValidHandle(std::vector<TI>& handles,
        uint32_t handleIndex,
        uint32_t handleGeneration
    ) const {
        if (handleIndex >= handles.size()) {
            return false;
        } else {
            const auto& internal = handles[handleIndex];
            return internal.alive && internal.generation == handleGeneration;
        }
    }

    template <typename T, typename TI>
    requires std::is_base_of_v<GLInternal, TI>
    bool IsValidHandle(std::vector<TI>& handles, const ExternalHandle<T>& handle) const {
        return IsValidHandle(handles, handle.index, handle.generation);
    }

    template <typename T>
    bool IsEqualHandles(const ExternalHandle<T>& handle0, const ExternalHandle<T>& handle1) const {
        return handle0.index == handle1.index && handle0.generation == handle1.generation;
    }

    template <typename T>
    void PostDeleteHandle(GLInternal& internal, const ExternalHandle<T>& handle, std::vector<uint32_t>& frees) const {
        internal.alive = false;
        internal.generation++;
        frees.push_back(handle.index);
    }
private:
    std::vector<GLBuffer>           m_Buffers{};
    std::vector<GLTexture>          m_Textures{};
    std::vector<GLProgram>          m_Programs{};
    std::vector<GLRenderPipeline>   m_RenderPipelines{};
    std::vector<GLComputePipeline>  m_ComputePipelines{};
    std::vector<GLFramebuffer>      m_Framebuffers{};
    std::vector<GLRenderPass>       m_RenderPasses{};
    std::vector<GLResourceSet>      m_ResourceSets{};

    std::vector<uint32_t>   m_FreeBuffers{};
    std::vector<uint32_t>   m_FreeTextures{};
    std::vector<uint32_t>   m_FreePrograms{};
    std::vector<uint32_t>   m_FreeRenderPipelines{};
    std::vector<uint32_t>   m_FreeComputePipelines{};
    std::vector<uint32_t>   m_FreeFramebuffers{};
    std::vector<uint32_t>   m_FreeRenderPasses{};
    std::vector<uint32_t>   m_FreeResourceSets{};

    GLCommandBinding<RenderPassHandle>       m_CurrentRenderPass{};
    GLCommandBinding<RenderPipelineHandle>   m_CurrentRenderPipeline{};
    GLCommandBinding<ComputePipelineHandle>  m_CurrentComputePipeline{};

    FramebufferHandle  m_DefaultBackbuffer{};
    GLSwapchain        m_Swapchain{};

    GLStateCache m_CurrentState{};

    Slang::ComPtr<slang::IGlobalSession> m_SlangGlobal{nullptr};
};

} // namespace axle::gfx
#endif