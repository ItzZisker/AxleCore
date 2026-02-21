#pragma once

#ifdef __AX_GRAPHICS_GL__

#include "AX_GLCommandList.hpp"

#include "axle/graphics/AX_Graphics.hpp"
#include "axle/utils/AX_Expected.hpp"

#include <slang.h>
#include <slang-com-ptr.h>

#include <glad/glad.h>

#include <type_traits>
#include <vector>

#ifdef AX_DEBUG
#include <iostream>

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

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#endif

#ifndef GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT 0x8C4D
#endif

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif

#ifndef GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT 0x8C4F
#endif

#ifndef GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT
#define GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT 0x8E8F
#endif

#ifndef GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT
#define GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT 0x8E8E
#endif

#ifndef GL_COMPRESSED_RGBA_BPTC_UNORM
#define GL_COMPRESSED_RGBA_BPTC_UNORM 0x8E8C
#endif

#ifndef GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM
#define GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM 0x8E8D
#endif

#ifndef GL_COMPRESSED_RGBA_ASTC_4x4_KHR
#define GL_COMPRESSED_RGBA_ASTC_4x4_KHR 0x93B0
#endif

#ifndef GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR 0x93D0
#endif

#ifndef GL_COMPRESSED_RGBA_ASTC_6x6_KHR
#define GL_COMPRESSED_RGBA_ASTC_6x6_KHR 0x93B4
#endif

#ifndef GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR 0x93D4
#endif

#ifndef GL_COMPRESSED_RGBA_ASTC_8x8_KHR
#define GL_COMPRESSED_RGBA_ASTC_8x8_KHR 0x93B7
#endif

#ifndef GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR 0x93D7
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

class GLGraphicsBackend final : public IGraphicsBackend {
private:
    GraphicsCaps m_Capabilities{};
    bool m_IsCore{false};
public:
    GLGraphicsBackend();
    ~GLGraphicsBackend() override;

    bool IsES() const { return !m_IsCore; }
    bool IsCore() const { return m_IsCore; }

    bool SupportsCap(GraphicsCapEnum cap) override;
    utils::ExResult<GraphicsCaps> QueryCaps() override;

    utils::ExResult<BufferHandle> CreateBuffer(const BufferDesc& desc) override;
    utils::AXError UpdateBuffer(BufferHandle& handle, size_t offset, size_t size, const void* data) override;
    utils::AXError DestroyBuffer(BufferHandle& handle) override;

    utils::ExResult<TextureHandle> CreateTexture(const TextureDesc& desc) override;
    utils::AXError UpdateTexture(TextureHandle& handle, const TextureSubDesc& subDesc, const void* data) override;
    utils::AXError DestroyTexture(TextureHandle& handle) override;

    utils::ExResult<FramebufferHandle> CreateFramebuffer(const FramebufferDesc& handle) override;
    utils::AXError DestroyFramebuffer(FramebufferHandle& handle) override;

    utils::ExResult<ShaderHandle> CreateProgram(const ShaderDesc& desc) override;
    utils::AXError DestroyProgram(ShaderHandle& handle) override;

    utils::ExResult<RenderPipelineHandle> CreateRenderPipeline(const RenderPipelineDesc& desc) override;
    utils::AXError DestroyRenderPipeline(RenderPipelineHandle& handle) override;

    utils::ExResult<ComputePipelineHandle> CreateComputePipeline(const ComputePipelineDesc& desc) override;
    utils::AXError DestroyComputePipeline(ComputePipelineHandle& handle) override;

    utils::ExResult<RenderPassHandle> CreateRenderPass(const RenderPassDesc& desc) override;
    utils::AXError DestroyRenderPass(RenderPassHandle& handle) override;

    utils::AXError Dispatch(ICommandList& cmd, uint32_t x, uint32_t y, uint32_t z) override;
    utils::AXError BindResources(ICommandList& cmd, Span<Binding> bindings) override;
    utils::AXError Barrier(ICommandList& cmd, Span<ResourceTransition> transitions) override;

    // Execution
    utils::AXError Execute(const GLCommandList& cmd);

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
            handles.emplace_back({.index = index});
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
    void PostDeleteHandle(GLInternal& internal, ExternalHandle<T>& handle, std::vector<uint32_t>& frees) const {
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

    std::vector<uint32_t> m_FreeBuffers{};
    std::vector<uint32_t> m_FreeTextures{};
    std::vector<uint32_t> m_FreePrograms{};
    std::vector<uint32_t> m_FreeRenderPipelines{};
    std::vector<uint32_t> m_FreeComputePipelines{};
    std::vector<uint32_t> m_FreeFramebuffers{};
    std::vector<uint32_t> m_FreeRenderPasses{};
    
    GLCommandBinding<RenderPassHandle>       m_CurrentRenderPass{};
    GLCommandBinding<RenderPipelineHandle>   m_CurrentRenderPipeline{};
    GLCommandBinding<ComputePipelineHandle>  m_CurrentComputePipeline{};

    GLStateCache m_CurrentState{};

    Slang::ComPtr<slang::IGlobalSession> m_SlangGlobal{nullptr};
};

} // namespace axle::gfx
#endif