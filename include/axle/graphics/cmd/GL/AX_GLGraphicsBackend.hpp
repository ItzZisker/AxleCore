#pragma once

#ifdef __AX_GRAPHICS_GL__

#include "AX_GLCommandList.hpp"

#include "axle/graphics/AX_Graphics.hpp"
#include "axle/utils/AX_Expected.hpp"

#include <slang.h>
#include <slang-com-ptr.h>

#include <glad/glad.h>

#include <type_traits>
#include <utility>
#include <vector>

#ifdef AX_DEBUG
    #define GL_CALL(x) \
        do { \
            while (glGetError() != GL_NO_ERROR); \
            x; \
            GLenum err = glGetError(); \
            if (err != GL_NO_ERROR) { \
                ReportGLError(err, #x, __FILE__, __LINE__); \
            } \
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

GLenum TTypeGLTarget(TextureType type);

GLenum TFmtGLInternalFormat(TextureFormat fmt);
GLenum TFmtGLFormat(TextureFormat fmt);
GLenum TFmtGLType(TextureFormat fmt);

GLenum TwGLWrap(TextureWrap wrap);
GLenum TfGLFilter(TextureFilter filter);

GLenum StgGLStage(ShaderStage stg);

GLenum AttrTGLType(VertexAttributeType type);

struct GLInternal {
    uint32_t generation{1};
    bool alive{false};
};

struct GLVertexInput : public GLInternal {
    GLuint id;
    VertexInputDesc desc;
};

struct GLBuffer : public GLInternal {
    GLuint id;
    GLenum target;
};

struct GLProgram : public GLInternal {
    GLuint id;
};

struct GLPipeline : public GLInternal {
    GLProgram program;
};

struct GLTexture : public GLInternal {
    GLuint id;
    TextureDesc desc;
};

struct GLFramebuffer : public GLInternal {
    GLuint fbo = 0;
    GLuint depthRbo = 0;
    uint32_t width = 0;
    uint32_t height = 0;
};

class GLGraphicsBackend final : public IGraphicsBackend {
private:
    GraphicsCaps m_Capabilities{};
public:
    GLGraphicsBackend();
    ~GLGraphicsBackend() override;

    bool SupportsCap(GraphicsCapEnum cap) override;
    utils::ExResult<GraphicsCaps> QueryCaps() override;

    utils::ExResult<BufferHandle> CreateBuffer(const BufferDesc& desc) override;
    utils::AXError UpdateBuffer(BufferHandle handle, size_t offset, size_t size, const void* data) override;
    utils::AXError DestroyBuffer(BufferHandle& handle) override;

    utils::ExResult<TextureHandle> CreateTexture(const TextureDesc& desc) override;
    utils::AXError UpdateTexture(TextureHandle handle, const TextureSubDesc& subDesc, const void* data) override;
    utils::AXError DestroyTexture(TextureHandle& handle) override;

    utils::ExResult<FramebufferHandle> CreateFramebuffer(const FramebufferDesc& handle) override;
    utils::AXError DestroyFramebuffer(FramebufferHandle handle) override;

    utils::ExResult<ShaderHandle> CreateProgram(const ShaderDesc& desc) override;
    utils::AXError DestroyProgram(ShaderHandle& handle) override;

    utils::ExResult<PipelineHandle> CreatePipeline(const PipelineDesc& desc) override;
    utils::AXError DestroyPipeline(PipelineHandle& handle) override;

    utils::ExResult<VertexInputHandle> CreateVertexInput(const VertexInputDesc& desc) override;
    utils::AXError DestroyVertexInput(VertexInputHandle& handle) override;

    utils::ExResult<RenderPassHandle> CreateRenderPass(const RenderPassDesc& desc) override;

    utils::AXError Dispatch(ICommandList& cmd, uint32_t x, uint32_t y, uint32_t z) override;
    utils::AXError BindResources(ICommandList& cmd, Span<Binding> bindings) override;
    utils::AXError Barrier(ICommandList&, ResourceHandle) override { return utils::AXError::NoError(); }

    // Execution
    void Execute(const GLCommandList& cmd);

    template <typename T>
    requires std::is_base_of_v<GLInternal, T>
    inline std::pair<uint32_t, T&> ReserveHandle(
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
        }
        return {index, handles[index]};
    }

    template <typename T, typename TI>
    requires std::is_base_of_v<GLInternal, TI>
    bool IsValidHandle(std::vector<TI>& handles, const GfxHandle<T>& handle) const {
        if (handle.index >= handles.size()) {
            return false;
        } else {
            const auto& internal = handles[handle.index];
            return internal.alive && internal.generation == handle.generation;
        }
    }

    template <typename T>
    void PostDeleteHandle(GLInternal& internal, GfxHandle<T>& handle, std::vector<uint32_t>& frees) const {
        internal.alive = false;
        internal.generation++;
        frees.push_back(handle.index);
    }
private:
    std::vector<GLBuffer>      m_Buffers{};
    std::vector<GLTexture>     m_Textures{};
    std::vector<GLProgram>     m_Programs{};
    std::vector<GLPipeline>    m_Pipelines{};
    std::vector<GLVertexInput> m_VertexInputs{};
    std::vector<GLFramebuffer> m_Framebuffers{};

    std::vector<uint32_t> m_FreeBuffers{};
    std::vector<uint32_t> m_FreeTextures{};
    std::vector<uint32_t> m_FreePrograms{};
    std::vector<uint32_t> m_FreePipelines{};
    std::vector<uint32_t> m_FreeVertexInputs{};
    std::vector<uint32_t> m_FreeFramebuffers{};

    Slang::ComPtr<slang::IGlobalSession> m_SlangGlobal{nullptr};
};

} // namespace axle::gfx
#endif