#pragma once

#ifdef __AX_GRAPHICS_GL__

#include "AX_GLCommandList.hpp"

#include "axle/graphics/ctx/AX_IRenderContext.hpp"
#include "axle/graphics/AX_Graphics.hpp"

#include "axle/utils/AX_Expected.hpp"
#include "axle/utils/AX_MagicPool.hpp"

#include <slang.h>
#include <slang-com-ptr.h>

#include <glad/gl.h>

#include <type_traits>
#include <vector>

#define AX_DEBUG
#ifdef AX_DEBUG
#include <iostream>

inline const char* GLErrorToString(GLenum err) {
    switch (err) {
        case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
        case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
        case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
        default: return "UNKNOWN_ERROR";
    }
}

#define GL_CALL(x)                                          \
    do {                                                    \
        while (m_GL->GetError() != GL_NO_ERROR);            \
        x;                                                  \
        GLenum err;                                         \
        while ((err = m_GL->GetError()) != GL_NO_ERROR) {   \
            std::cerr << "GL Error: "                       \
                    << err                                  \
                    << ": "                                 \
                    << GLErrorToString(err)                 \
                    << " in " << #x                         \
                    << " at " << __FILE__                   \
                    << ":" << __LINE__                      \
                    << std::endl;                           \
            throw std::runtime_error("GL Error");           \
        }                                                   \
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

GLenum ToGLTextureTarget(TextureType type);
GLenum ToGLTextureInternalFormat(TextureFormat fmt);
GLenum ToGLTextureFormat(TextureFormat fmt);
GLenum ToGLTextureType(TextureFormat fmt);
GLenum ToGLTextureWrap(TextureWrap wrap);
GLenum ToGLMinFilter(TextureFilter f, MipmapFilter mf);
GLenum ToGLMagFilter(TextureFilter f);
GLenum ToGLShaderStage(ShaderStage stg);
GLenum ToGLVertexAttribType(VertexAttributeType type);
GLenum ToGLCompare(CompareOp cmpop);
GLenum ToGLBlendFactor(BlendFactor bFactor);
GLenum ToGLBlendOp(BlendOp bOp);
GLenum ToGLStencilOp(StencilOp sOp);
GLenum ToGLBarrierBit(ResourceState state);
GLenum ToGLPolyMode(PolyMode polyMode);
GLenum ToGLBufferTarget(BufferUsage usage);
GLenum ToGLBufferAccess(BufferAccess access);

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

template <typename T_Extern>
struct GLInternal : public utils::MagicInternal<T_Extern> {};

struct GLProgram : public GLInternal<ShaderHandle> {
    GLuint id{0};
};

struct GLBuffer : public GLInternal<BufferHandle> {
    GLuint id{0};
    BufferUsage usage;
    size_t size{0};
    uint32_t slot{0};
};

struct VAOKey {
    GLuint vbo;
    GLuint ebo;

    bool operator==(const VAOKey& other) const noexcept {
        return vbo == other.vbo && ebo == other.ebo;
    }
};

struct VAOKeyLookup {
    size_t operator()(const VAOKey& k) const {
        return (std::hash<GLuint>()(k.vbo)) ^
               (std::hash<GLuint>()(k.ebo) << 1);
    }
};

struct GLRenderPipeline : public GLInternal<RenderPipelineHandle> {
    RenderPipelineDesc desc;
    std::unordered_map<VAOKey, GLuint, VAOKeyLookup> vaoCache{};
};

struct GLComputePipeline : public GLInternal<ComputePipelineHandle> {
    ShaderHandle program;
    ComputePipelineDesc desc;
};

struct GLTexture : public GLInternal<TextureHandle> {
    GLuint id{0};
    GLuint resolveId{0};
    TextureDesc desc;
};

struct GLRenderPass : public GLInternal<RenderPassHandle> {
    RenderPassDesc desc;
    GLCommandBinding<FramebufferHandle> fbInUse{};
};

struct GLFramebuffer : public GLInternal<FramebufferHandle> {
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

struct GLResourceSet : public GLInternal<ResourceSetHandle> {
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
    bool stencilTest{false};
    bool depthWrite{true};
    GLenum depthFunc{GL_LESS};

    bool blendEnabled{false};
    GLenum srcColor{GL_ONE};
    GLenum dstColor{GL_ZERO};
    GLenum srcAlpha{GL_ONE};
    GLenum dstAlpha{GL_ZERO};
    GLenum blendColorOp{GL_FUNC_ADD};
    GLenum blendAlphaOp{GL_FUNC_ADD};

    GLuint currentVao{0};
    GLuint currentVbo{0};
    GLuint currentEbo{0};
};

/*
    TODO:
    - Make ThreadContextAudio for supporting audio stream ticking, decoding and playing sound ques asynch.
    - Implement logger like spdlog, based on previous java codes of MiddleWare Console, (Flushing, Compiled Patterns, ANSI) and log Vulkan/GL errors through that or engine-related stuff
*/

class GLGraphicsBackend final : public IGraphicsBackend {
private:
    SurfaceInfo m_SurfaceInfo{};
    SharedPtr<gfx::IRenderContext> m_Context{nullptr};
    GladGLContext* m_GL{nullptr};

    GraphicsCaps m_Capabilities{};
    bool m_IsCore{false};

    utils::ExError InternalExecute(CommandType type, data::BufferDataStream& args);

    utils::ExResult<GLuint> CreateVertexArray(GLRenderPipeline& pipeline);
    utils::ExError PrepVertexArray(GLRenderPipeline& pipeline);
public:
    GLGraphicsBackend(gfx::IRenderContext* context);
    ~GLGraphicsBackend() override;

    SharedPtr<gfx::IRenderContext> GetContext() const { return m_Context; }

    bool IsES() const { return !m_IsCore; }
    bool IsCore() const { return m_IsCore; }

    const GraphicsCaps& GetCaps() const override;
    bool SupportsCap(GraphicsCapEnum cap) override;
    utils::ExResult<GraphicsCaps> QueryCaps() override;

    utils::ExResult<SwapchainHandle> CreateSwapchain(const SwapchainDesc& desc) override;
    utils::ExError DestroySwapchain(const SwapchainHandle& desc) override;
    utils::ExError ResizeSwapchain(const SwapchainHandle& desc, uint32_t width, uint32_t height) override;

    utils::ExResult<BufferHandle> CreateBuffer(const BufferDesc& desc) override;
    utils::ExError UpdateBuffer(const BufferHandle& handle, size_t offset, size_t size, const void* data) override;
    utils::ExError DestroyBuffer(const BufferHandle& handle) override;

    utils::ExResult<TextureHandle> CreateTexture(const TextureDesc& desc) override;
    utils::ExError UpdateTexture(const TextureHandle& handle, const TextureSubDesc& subDesc, const void* data) override;
    utils::ExError DestroyTexture(const TextureHandle& handle) override;

    utils::ExResult<FramebufferHandle> CreateFramebuffer(const FramebufferDesc& handle) override;
    utils::ExError DestroyFramebuffer(const FramebufferHandle& handle) override;

    utils::ExResult<ShaderHandle> CreateProgram(const ShaderDesc& desc) override;
    utils::ExError DestroyProgram(const ShaderHandle& handle) override;

    utils::ExResult<RenderPipelineHandle> CreateRenderPipeline(const RenderPipelineDesc& desc) override;
    utils::ExError DestroyRenderPipeline(const RenderPipelineHandle& handle) override;

    utils::ExResult<ComputePipelineHandle> CreateComputePipeline(const ComputePipelineDesc& desc) override;
    utils::ExError DestroyComputePipeline(const ComputePipelineHandle& handle) override;

    utils::ExResult<RenderPassHandle> CreateDefaultRenderPass(const DefaultRenderPassDesc& desc) override;
    utils::ExResult<RenderPassHandle> CreateRenderPass(const RenderPassDesc& desc) override;
    utils::ExError DestroyRenderPass(const RenderPassHandle& handle) override;

    utils::ExResult<ResourceSetHandle> CreateResourceSet(const ResourceSetDesc& desc) override;
    utils::ExError UpdateResourceSet(const ResourceSetHandle& handle, std::vector<Binding> bindings) override;
    utils::ExError DestroyResourceSet(const ResourceSetHandle& handle) override;

    utils::ExError Execute(ICommandList& cmd) override;
    utils::ExError Dispatch(ICommandList& cmd, uint32_t x, uint32_t y, uint32_t z) override;
    utils::ExError Barrier(ICommandList& cmd, std::vector<ResourceTransition> transitions) override;

    utils::ExResult<uint32_t> AcquireNextImage() override;
    utils::ExResult<FramebufferHandle> GetSwapchainFramebuffer(uint32_t imageIndex) override;
    utils::ExError Present(uint32_t imageIndex) override;
private:
    utils::MagicPool<GLBuffer>           m_Buffers{};
    utils::MagicPool<GLTexture>          m_Textures{};
    utils::MagicPool<GLProgram>          m_Programs{};
    utils::MagicPool<GLRenderPipeline>   m_RenderPipelines{};
    utils::MagicPool<GLComputePipeline>  m_ComputePipelines{};
    utils::MagicPool<GLFramebuffer>      m_Framebuffers{};
    utils::MagicPool<GLRenderPass>       m_RenderPasses{};
    utils::MagicPool<GLResourceSet>      m_ResourceSets{};

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