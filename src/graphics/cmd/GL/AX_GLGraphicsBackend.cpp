#ifdef __AX_GRAPHICS_GL__

#include "axle/graphics/cmd/GL/AX_GLCommandList.hpp"
#include "axle/graphics/cmd/GL/AX_GLGraphicsBackend.hpp"

#ifdef __AX_PLATFORM_WIN32__
#include "axle/graphics/ctx/GL/AX_RenderContextGLWin32.hpp"
#elif defined(__AX_PLATFORM_X11__)
#include "axle/graphics/ctx/GL/AX_RenderContextGLX11.hpp"
#endif

#include "axle/graphics/AX_Graphics.hpp"

#include "axle/utils/AX_Expected.hpp"

#include <glad/gl.h>

#include <slang.h>
#include <slang-com-ptr.h>
#include <slang-com-helper.h>

#include <unordered_set>
#include <cstddef>
#include <string>
#include <cstring>
#include <vector>

using namespace axle::utils;

namespace axle::gfx {

GLGraphicsBackend::GLGraphicsBackend(IRenderContext* context) {
    m_Context = SharedPtr<IRenderContext>(context);

    slang::createGlobalSession(m_SlangGlobal.writeRef());

#ifdef __AX_PLATFORM_WIN32__
    auto handle = std::static_pointer_cast<GLCHandleWin32>(m_Context->GetContextHandle());
    m_GL = &handle->glCtx;
#elif defined(__AX_PLATFORM_X11__)
    auto handle = std::static_pointer_cast<GLCHandleX11>(m_Context->GetContextHandle());
    m_GL = &handle->glCtx;
#endif
    m_Capabilities = ExpectOrThrow(QueryCaps());

    m_SurfaceInfo.depthBits = handle->surfaceInfo.depthBits;
    m_SurfaceInfo.stencilBits = handle->surfaceInfo.stencilBits;
    m_SurfaceInfo.width = handle->surfaceInfo.width;
    m_SurfaceInfo.height = handle->surfaceInfo.height;
    m_SurfaceInfo.vsync = handle->surfaceInfo.vsync;

    // emulate swapchain as a single backbuffer for GL
    auto& fb = *m_Framebuffers.Reserve();
    
    fb.fbo = 0;
    fb.hasResolve = false;
    fb.resolveFbo = 0;
    fb.renderPass = {};

    int sampleBuffers = 0;
    int samples = 0;
    m_GL->GetIntegerv(GL_SAMPLE_BUFFERS, &sampleBuffers);
    m_GL->GetIntegerv(GL_SAMPLES, &samples);

    printf("Sample buffers = %d, samples = %d\n", sampleBuffers, samples);

    fb.width = m_SurfaceInfo.width;
    fb.height = m_SurfaceInfo.height;

    fb.hasDepth = m_SurfaceInfo.depthBits > 0;
    fb.hasStencil = m_SurfaceInfo.stencilBits > 0;
    fb.depthStencilTexture = 0; // default FBO owns it
    fb.isSwapchain = true;
    fb.alive = true;

    m_DefaultBackbuffer = fb.External();

    m_Swapchain.width = fb.width;
    m_Swapchain.height = fb.height;
    m_Swapchain.format = TextureFormat::RGBA8_UINT;
    m_Swapchain.backbuffer = m_DefaultBackbuffer;
}

GLGraphicsBackend::~GLGraphicsBackend() {
    slang::shutdown();
}

const GraphicsCaps& GLGraphicsBackend::GetCaps() const {
    return m_Capabilities;
}

bool GLGraphicsBackend::SupportsCap(GraphicsCapEnum cap) {
    return m_Capabilities.caps[static_cast<int32_t>(cap)];
};

ExResult<GraphicsCaps> GLGraphicsBackend::QueryCaps() {
    GraphicsCaps result{};

    const char* versionStr = (const char*)m_GL->GetString(GL_VERSION);
    m_IsCore = strstr(versionStr, "OpenGL ES") == nullptr;
    bool isGL = m_IsCore, isES = !isGL;

    int major = 0, minor = 0;
    GL_CALL(m_GL->GetIntegerv(GL_MAJOR_VERSION, &major));
    GL_CALL(m_GL->GetIntegerv(GL_MINOR_VERSION, &minor));

    std::unordered_set<std::string> extensions;

    GLint extn = 0;
    GL_CALL(m_GL->GetIntegerv(GL_NUM_EXTENSIONS, &extn));
    for (GLint i = 0; i < extn; i++) {
        extensions.insert((const char*) m_GL->GetStringi(GL_EXTENSIONS, i));
    }
    auto HasExt = [&](const char* ext) {
        return extensions.contains(ext);
    };

    result.caps[(int)GraphicsCapEnum::ComputeShaders] =
        (isGL  && (major > 4 || (major == 4 && minor >= 3))) ||
        (isES  && (major > 3 || (major == 3 && minor >= 1))) ||
        HasExt("GL_ARB_compute_shader");

    result.caps[(int)GraphicsCapEnum::ShaderStorageBuffers] =
        (isGL  && (major > 4 || (major == 4 && minor >= 3))) ||
        (isES  && (major > 3 || (major == 3 && minor >= 1))) ||
        HasExt("GL_ARB_shader_storage_buffer_object");

    result.caps[(int)GraphicsCapEnum::IndirectDraw] =
        (isGL && (major >= 4)) ||
        HasExt("GL_ARB_draw_indirect");

    result.caps[(int)GraphicsCapEnum::GeometryShaders] =
        (isGL && (major > 3 || (major == 3 && minor >= 2))) ||
        (isES && (major > 3 || (major == 3 && minor >= 2))) ||
        HasExt("GL_ARB_geometry_shader4");

    result.caps[(int)GraphicsCapEnum::Tessellation] =
        (isGL && (major > 4 || (major == 4 && minor >= 0))) ||
        (isES && (major > 3 || (major == 3 && minor >= 2))) ||
        HasExt("GL_ARB_tessellation_shader");

    result.caps[(int)GraphicsCapEnum::MultiDrawIndirect] =
        (isGL && (major > 4 || (major == 4 && minor >= 3))) ||
        HasExt("GL_ARB_multi_draw_indirect");

    result.caps[(int)GraphicsCapEnum::SparseTextures] =
        (isGL && (major > 4 || (major == 4 && minor >= 4))) ||
        HasExt("GL_ARB_sparse_texture");

    result.caps[(int)GraphicsCapEnum::RayTracing] =
        HasExt("GL_NV_ray_tracing");

    result.caps[(int)GraphicsCapEnum::BindlessTextures] =
        HasExt("GL_ARB_bindless_texture") ||
        HasExt("GL_NV_bindless_texture");

    result.caps[(int)GraphicsCapEnum::HalfFloatColorBuffer] =
        (isGL) ||
        (isES && HasExt("GL_EXT_color_buffer_half_float"));

    result.caps[(int)GraphicsCapEnum::FullFloatColorBuffer] =
        (isGL) ||
        (isES && HasExt("GL_EXT_color_buffer_float"));

    result.caps[(int)GraphicsCapEnum::LongPointers] =
        (isGL && (major > 4 || (major == 4 && minor >= 1))) ||
        HasExt("GL_ARB_vertex_attrib_64bit");

    result.caps[(int)GraphicsCapEnum::Anisotropy] = 
        HasExt("GL_EXT_texture_filter_anisotropic") ||
        HasExt("GL_ARB_texture_filter_anisotropic");;

    GL_CALL(m_GL->GetIntegerv(GL_MAX_VERTEX_ATTRIBS, &result.maxVertexAttribs));

    GL_CALL(m_GL->GetIntegerv(GL_MAX_DRAW_BUFFERS, &result.maxDrawBuffers));
    GL_CALL(m_GL->GetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &result.maxColorAttachments));

    GL_CALL(m_GL->GetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &result.maxUBOBindings));
    GL_CALL(m_GL->GetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &result.maxUBOSize));

    GL_CALL(m_GL->GetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &result.maxArrayTextureLayers));
    GL_CALL(m_GL->GetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &result.maxTextureUnits));
    GL_CALL(m_GL->GetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &result.maxCombinedTextureUnits));

    if (result.Has(GraphicsCapEnum::ShaderStorageBuffers)) {
        GL_CALL(m_GL->GetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &result.maxSSBOSize));
        GL_CALL(m_GL->GetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &result.maxSSBOBindings));
    }
    if (result.Has(GraphicsCapEnum::ComputeShaders)) {
        for (int i = 0; i < 3; ++i) {
            GL_CALL(m_GL->GetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, i, (int*)&result.maxWorkGroupCount[i]));
        }
    }
    if (result.Has(GraphicsCapEnum::Anisotropy)) {
        GL_CALL(m_GL->GetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &result.maxAniso));
    }

    return result;
}

utils::ExResult<SwapchainHandle> GLGraphicsBackend::CreateSwapchain(const SwapchainDesc&) {
    return SwapchainHandle{0, 1};
}

utils::ExError GLGraphicsBackend::DestroySwapchain(const SwapchainHandle&) {
    // GL implicit
    return ExError::NoError();
}

utils::ExError GLGraphicsBackend::ResizeSwapchain(const SwapchainHandle&, uint32_t width, uint32_t height) {
    m_Swapchain.width = width;
    m_Swapchain.height = height;
    return ExError::NoError();
}

ExResult<BufferHandle> GLGraphicsBackend::CreateBuffer(const BufferDesc& desc) {
    if (desc.usage == BufferUsage::Storage && !m_Capabilities.Has(GraphicsCapEnum::ShaderStorageBuffers))
        return ExError{"Storage buffers not supported on this OpenGL backend (requires 4.3+)"};

    if (desc.usage == BufferUsage::Indirect && !m_Capabilities.Has(GraphicsCapEnum::IndirectDraw))
        return ExError{"Indirect buffers not supported on this backend"};

    // Staging buffer is fine in GL 3.3 (just normal array buffer with CPU updates)
    auto& buff = *m_Buffers.Reserve();

    GLuint id = 0;
    GLenum target = ToGLBufferTarget(desc.usage);

    GL_CALL(m_GL->GenBuffers(1, &id));
    GL_CALL(m_GL->BindBuffer(target, id));
    GL_CALL(m_GL->BufferData(target, desc.size, nullptr, ToGLBufferAccess(desc.access)));

    buff.id = id;
    buff.usage = desc.usage;
    buff.size = desc.size;
    buff.slot = desc.bindSlot;
    buff.alive = true;

    return buff.External();
}

ExError GLGraphicsBackend::UpdateBuffer(const BufferHandle& handle, size_t offset, size_t size, const void* data) {
    if (!m_Buffers.IsValid(handle))
        return {"Invalid Handle"};

    auto& buff = *m_Buffers.Get(handle);

    if (offset + size > buff.size)
        return {"UpdateBuffer out of bounds"};

    GLenum target = ToGLBufferTarget(buff.usage);
    GL_CALL(m_GL->BindBuffer(target, buff.id));
    GL_CALL(m_GL->BufferSubData(target, offset, size, data));

    return ExError::NoError();
}

ExError GLGraphicsBackend::DestroyBuffer(const BufferHandle& handle) {
    if (!m_Buffers.IsValid(handle))
        return {"Invalid Handle"};

    auto& buff = *m_Buffers.Get(handle);

    GL_CALL(m_GL->DeleteBuffers(1, &buff.id));

    buff.id = 0;
    m_Buffers.Delete(handle);
    return ExError::NoError();
}

static uint32_t CalcFullMipCount(uint32_t w, uint32_t h, uint32_t d = 1) {
    uint32_t size = std::max({w, h, d});
    uint32_t levels = 1;
    while (size > 1) {
        size >>= 1;
        ++levels;
    }
    return levels;
}

ExResult<TextureHandle> GLGraphicsBackend::CreateTexture(const TextureDesc& desc) {
    if (desc.width == 0 || desc.height == 0)
        return ExError{"Invalid dimensions"};

    auto& tex = *m_Textures.Reserve();
    tex.desc = desc;

    GLenum target = ToGLTextureTarget(desc.type);
    GLenum internalFormat = ToGLTextureInternalFormat(desc.format);
    GLenum format = ToGLTextureFormat(desc.format);
    GLenum type = ToGLTextureType(desc.format);
    bool compressed = TextureFormatIsS3TC(desc.format) || TextureFormatIsASTC(desc.format);

    uint32_t mip = desc.subDesc.mipLevel;
    if (mip == 0) {
        uint32_t depth = (desc.type == TextureType::Texture3D) ? desc.depth : 1;
        mip = CalcFullMipCount(desc.width, desc.height, depth);
    }

    // Allocate and bind
    GL_CALL(m_GL->GenTextures(1, &tex.id));
    GL_CALL(m_GL->BindTexture(target, tex.id));
    GL_CALL(m_GL->PixelStorei(GL_UNPACK_ALIGNMENT, 1));

    // Upload base level (mip 0)
    switch (desc.type) {
        case TextureType::Texture2D: {
            const void* data = desc.pixelsByLayers[0].size() == 0 ? nullptr : desc.pixelsByLayers[0].data();
            GLsizei dataSize = (GLsizei)desc.pixelsByLayers[0].size();

            if (compressed) {
                GL_CALL(m_GL->CompressedTexImage2D(
                    target, 0, internalFormat,
                    desc.width, desc.height, 0,
                    dataSize, data
                ));
            } else {
                GL_CALL(m_GL->TexImage2D(
                    target, 0, internalFormat,
                    desc.width, desc.height, 0,
                    format, type, data
                ));
            }
        } break;

        case TextureType::Array2D: {
            const void* data = desc.pixelsByLayers[0].size() == 0 ? nullptr : desc.pixelsByLayers[0].data();
            GLsizei dataSize = (GLsizei)desc.pixelsByLayers[0].size();

            if (compressed) {
                GL_CALL(m_GL->CompressedTexImage3D(
                    target, 0, internalFormat,
                    desc.width, desc.height, desc.layers, 0,
                    dataSize, data
                ));
            } else {
                GL_CALL(m_GL->TexImage3D(
                    target, 0, internalFormat,
                    desc.width, desc.height, desc.layers, 0,
                    format, type, data
                ));
            }
        } break;

        case TextureType::Texture3D: {
            const void* data = desc.pixelsByLayers[0].size() == 0 ? nullptr : desc.pixelsByLayers[0].data();
            GLsizei dataSize = (GLsizei)desc.pixelsByLayers[0].size();

            if (compressed) {
                GL_CALL(m_GL->CompressedTexImage3D(
                    target, 0, internalFormat,
                    desc.width, desc.height, desc.depth, 0,
                    dataSize, data
                ));
            } else {
                GL_CALL(m_GL->TexImage3D(
                    target, 0, internalFormat,
                    desc.width, desc.height, desc.depth, 0,
                    format, type, data
                ));
            }
        } break;

        case TextureType::Cubemap: {
            if (desc.pixelsByLayers.size() != 6) {
                GL_CALL(m_GL->DeleteTextures(1, &tex.id));
                return ExError{"Invalid pixelsByLayers, should be size of 6, each representing a side of cubemap"};
            }
            for (uint32_t i = 0; i < 6; ++i) {
                const void* data = desc.pixelsByLayers[i].size() == 0
                    ? nullptr
                    : desc.pixelsByLayers[i].data();
                GLsizei dataSize = (GLsizei)desc.pixelsByLayers[i].size();

                if (compressed) {
                    GL_CALL(m_GL->CompressedTexImage2D(
                        GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                        0, internalFormat,
                        desc.width, desc.height, 0,
                        dataSize, data
                    ));
                } else {
                    GL_CALL(m_GL->TexImage2D(
                        GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                        0, internalFormat,
                        desc.width, desc.height, 0,
                        format, type, data
                    ));
                }
            }
        } break;
    }

    // Sampler state
    GL_CALL(m_GL->TexParameteri(target, GL_TEXTURE_WRAP_S, ToGLTextureWrap(desc.subDesc.wrapS)));
    GL_CALL(m_GL->TexParameteri(target, GL_TEXTURE_WRAP_T, ToGLTextureWrap(desc.subDesc.wrapT)));
    GL_CALL(m_GL->TexParameteri(target, GL_TEXTURE_WRAP_R, ToGLTextureWrap(desc.subDesc.wrapR)));

    GLenum glMin = ToGLMinFilter(desc.subDesc.minFilter, desc.subDesc.mipFilter);
    GLenum glMag = ToGLMagFilter(desc.subDesc.magFilter);

    GL_CALL(m_GL->TexParameteri(target, GL_TEXTURE_MIN_FILTER, glMin));
    GL_CALL(m_GL->TexParameteri(target, GL_TEXTURE_MAG_FILTER, glMag));

    // Border
    if (desc.subDesc.wrapS == TextureWrap::ClampToBorder ||
        desc.subDesc.wrapT == TextureWrap::ClampToBorder ||
        desc.subDesc.wrapR == TextureWrap::ClampToBorder)
    {
        GL_CALL(m_GL->TexParameterfv(target, GL_TEXTURE_BORDER_COLOR, &desc.subDesc.borderColor.r));
    }

    // Anisotropy
    if (desc.subDesc.aniso > 0.0f) {
        float aniso = std::min(desc.subDesc.aniso, m_Capabilities.maxAniso);
        GL_CALL(m_GL->TexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso));
    }

    // Mip chain
    GL_CALL(m_GL->TexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0));
    GL_CALL(m_GL->TexParameteri(target, GL_TEXTURE_MAX_LEVEL, mip - 1));

    if (desc.subDesc.generateMips && mip > 1) {
        GL_CALL(m_GL->GenerateMipmap(target));
    }

    GL_CALL(m_GL->BindTexture(target, 0));

    tex.alive = true;
    return tex.External();
}

static TextureImageDescriptor Describe(const TextureDesc& desc, int mip) {
    TextureImageDescriptor imgDesc;
    imgDesc.type = desc.type;
    imgDesc.format = desc.format;
    imgDesc.width = desc.width;
    imgDesc.height = desc.height;
    imgDesc.layers = desc.layers;
    imgDesc.mip = mip;
    return imgDesc;
}

static GLenum GetCubemapFaceFromSubDesc(const TextureSubDesc& subDesc) {
    switch (subDesc.updateThisCubemapFace) {
        case TextureCubemapFace::NegativeX: return GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
        case TextureCubemapFace::NegativeY: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
        case TextureCubemapFace::NegativeZ: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
        case TextureCubemapFace::PositiveX: return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
        case TextureCubemapFace::PositiveY: return GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
        case TextureCubemapFace::PositiveZ: return GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
    }
    return GL_NONE;
}

ExError GLGraphicsBackend::UpdateTexture(const TextureHandle& handle, const TextureSubDesc& subDesc, const void* data) {
    if (!m_Textures.IsValid(handle))
        return {"Invalid Handle"};

    auto& tex = *m_Textures.Get(handle);
    auto& desc = tex.desc;

    GLenum target = ToGLTextureTarget(desc.type);
    GLenum format = ToGLTextureFormat(desc.format);
    GLenum type = ToGLTextureType(desc.format);
    GLenum internalFormat = ToGLTextureInternalFormat(desc.format);
    bool compressed = TextureFormatIsS3TC(desc.format) || TextureFormatIsASTC(desc.format);

    uint32_t mip = subDesc.mipLevel;

    GL_CALL(m_GL->BindTexture(target, tex.id));
    GL_CALL(m_GL->PixelStorei(GL_UNPACK_ALIGNMENT, 1));

    switch (desc.type) {
        case TextureType::Texture2D: {
            GLsizei imageSize = compressed ? CalcImageSize(Describe(desc, mip)) : 0;
            if (compressed) {
                GL_CALL(m_GL->CompressedTexSubImage2D(
                    target, mip,
                    0, 0,
                    desc.width >> mip,
                    desc.height >> mip,
                    internalFormat,
                    imageSize,
                    data
                ));
            } else {
                GL_CALL(m_GL->TexSubImage2D(
                    target, mip,
                    0, 0,
                    desc.width >> mip,
                    desc.height >> mip,
                    format, type,
                    data
                ));
            }
        } break;

        case TextureType::Array2D: {
            GLsizei imageSize = compressed ? CalcImageSize(Describe(desc, mip)) : 0;
            if (compressed) {
                GL_CALL(m_GL->CompressedTexSubImage3D(
                    target, mip,
                    0, 0, 0,
                    desc.width >> mip,
                    desc.height >> mip,
                    desc.layers,
                    internalFormat,
                    imageSize,
                    data
                ));
            } else {
                GL_CALL(m_GL->TexSubImage3D(
                    target, mip,
                    0, 0, 0,
                    desc.width >> mip,
                    desc.height >> mip,
                    desc.layers,
                    format, type,
                    data
                ));
            }
        } break;

        case TextureType::Texture3D: {
            GLsizei imageSize = compressed ? CalcImageSize(Describe(desc, mip)) : 0;
            if (compressed) {
                GL_CALL(m_GL->CompressedTexSubImage3D(
                    target, mip,
                    0, 0, 0,
                    desc.width >> mip,
                    desc.height >> mip,
                    desc.depth >> mip,
                    internalFormat,
                    imageSize,
                    data
                ));
            } else {
                GL_CALL(m_GL->TexSubImage3D(
                    target, mip,
                    0, 0, 0,
                    desc.width >> mip,
                    desc.height >> mip,
                    desc.depth >> mip,
                    format, type,
                    data
                ));
            }
        } break;

        case TextureType::Cubemap: {
            // Caller must pass data for one face only.
            GLenum face = GetCubemapFaceFromSubDesc(subDesc);
            GLsizei imageSize = compressed ? CalcImageSize(Describe(desc, mip)) : 0;

            if (compressed) {
                GL_CALL(m_GL->CompressedTexSubImage2D(
                    face, mip,
                    0, 0,
                    desc.width >> mip,
                    desc.height >> mip,
                    internalFormat,
                    imageSize,
                    data
                ));
            } else {
                GL_CALL(m_GL->TexSubImage2D(
                    face, mip,
                    0, 0,
                    desc.width >> mip,
                    desc.height >> mip,
                    format, type,
                    data
                ));
            }
        } break;
    }

    // Update sampler state
    GL_CALL(m_GL->TexParameteri(target, GL_TEXTURE_WRAP_S, ToGLTextureWrap(subDesc.wrapS)));
    GL_CALL(m_GL->TexParameteri(target, GL_TEXTURE_WRAP_T, ToGLTextureWrap(subDesc.wrapT)));
    GL_CALL(m_GL->TexParameteri(target, GL_TEXTURE_WRAP_R, ToGLTextureWrap(subDesc.wrapR)));

    GLenum glMin = ToGLMinFilter(subDesc.minFilter, subDesc.mipFilter);
    GLenum glMag = ToGLMagFilter(subDesc.magFilter);

    GL_CALL(m_GL->TexParameteri(target, GL_TEXTURE_MIN_FILTER, glMin));
    GL_CALL(m_GL->TexParameteri(target, GL_TEXTURE_MAG_FILTER, glMag));

    if (subDesc.wrapS == TextureWrap::ClampToBorder ||
        subDesc.wrapT == TextureWrap::ClampToBorder ||
        subDesc.wrapR == TextureWrap::ClampToBorder)
    {
        GL_CALL(m_GL->TexParameterfv(target, GL_TEXTURE_BORDER_COLOR, &subDesc.borderColor.r));
    }

    if (subDesc.aniso > 0.0f) {
        float aniso = std::min(subDesc.aniso, m_Capabilities.maxAniso);
        GL_CALL(m_GL->TexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso));
    }

    if (subDesc.generateMips)
        GL_CALL(m_GL->GenerateMipmap(target));

    tex.desc.subDesc = subDesc;

    GL_CALL(m_GL->BindTexture(target, 0));
    return ExError::NoError();
}

ExError GLGraphicsBackend::DestroyTexture(const TextureHandle& handle) {
    if (m_Textures.IsValid(handle))
        return {"Invalid Handle"};

    auto& tex = *m_Textures.Get(handle);
    GL_CALL(m_GL->DeleteTextures(1, &tex.id));
    tex.id = 0;
    m_Textures.Delete(handle);
    return ExError::NoError();
}

ExResult<FramebufferHandle> GLGraphicsBackend::CreateFramebuffer(const FramebufferDesc& desc) {
    if (!m_RenderPasses.IsValid(desc.renderPass))
        return ExError{"Invalid RenderPass handle"};

    auto& rp = *m_RenderPasses.Get(desc.renderPass);
    const RenderPassDesc& rpDesc = rp.desc;

    if (desc.colorAttachments.size() != rpDesc.colorAttachments.size())
        return ExError{"Framebuffer color attachment count does not match RenderPass"};

    for (uint32_t i = 0; i < desc.colorAttachments.size(); ++i) {
        auto texHandle = desc.colorAttachments[i];

        if (!m_Textures.IsValid(texHandle)) {
            return ExError{
                "Invalid Texture Handle at index " +
                std::to_string(i)
            };
        }

        auto& tex = *m_Textures.Get(texHandle);
        const AttachmentDesc& rpAtt = rpDesc.colorAttachments[i];

        if (tex.desc.samples != rpAtt.samples) {
            return ExError{
                "Color attachment sample count mismatch at index " +
                std::to_string(i)
            };
        }

        if (tex.desc.format != rpAtt.format) {
            return ExError{
                "Color attachment format mismatch at index " +
                std::to_string(i)
            };
        }

        GLenum glType = ToGLTextureType(tex.desc.format);

        if (glType == GL_HALF_FLOAT && !m_Capabilities.Has(GraphicsCapEnum::HalfFloatColorBuffer)) {
            return ExError{"Host doesn't support 16 bit float Color buffers (Unwritable)"};
        }
        if (glType == GL_FLOAT && !m_Capabilities.Has(GraphicsCapEnum::FullFloatColorBuffer)) {
            return ExError{"Host doesn't support 32 bit float Color buffers (Unwritable)"};
        }

        if (tex.desc.width != desc.width || tex.desc.height != desc.height) {
            return ExError{
                "Color attachment size mismatch at index " +
                std::to_string(i)
            };
        }
    }

    const auto& rpHasDepth = rpDesc.hasDepth;
    const auto& fbHasDepth = desc.hasDepth;

    const auto& rpHasStencil = rpDesc.hasStencil;
    const auto& fbHasStencil = desc.hasStencil;

    if (fbHasDepth != rpHasDepth) {
        return ExError{"Depth attachment presence mismatch between RenderPass and Framebuffer"};
    }

    if (fbHasStencil != rpHasStencil) {
        return ExError{"Stencil attachment presence mismatch between RenderPass and Framebuffer"};
    }

    const bool hasDepth = fbHasDepth && rpHasDepth;
    const bool hasStencil = fbHasStencil && rpHasStencil;

    if (hasDepth || hasStencil) {
        if (!m_Textures.IsValid(desc.depthStencilTexture))
            return ExError{"Invalid depth/stencil texture handle"};

        auto& depthStencilTex = *m_Textures.Get(desc.depthStencilTexture);
        const AttachmentDesc& rpDepthStencil = rpDesc.depthStencilAttachment;

        if (depthStencilTex.desc.format != rpDepthStencil.format)
            return ExError{"Depth/Stencil attachment format mismatch"};

        if (depthStencilTex.desc.width != desc.width ||
            depthStencilTex.desc.height != desc.height)
            return ExError{"Depth/Stencil attachment size mismatch"};
    }

    auto& fb = *m_Framebuffers.Reserve();

    GL_CALL(m_GL->GenFramebuffers(1, &fb.fbo));
    GL_CALL(m_GL->BindFramebuffer(GL_FRAMEBUFFER, fb.fbo));

    for (uint32_t i = 0; i < desc.colorAttachments.size(); ++i) {
        auto& tex = *m_Textures.Get(desc.colorAttachments[i]);

        GL_CALL(m_GL->FramebufferTexture2D(
            GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0 + i,
            ToGLTextureTarget(tex.desc.type),
            tex.id,
            0
        ));
    }

    if (hasDepth || hasStencil) {
        auto& depthStencilTex = *m_Textures.Get(desc.depthStencilTexture);
        GLenum attachmentGLType;

        if (hasDepth && hasStencil) {
            attachmentGLType = GL_DEPTH_STENCIL_ATTACHMENT;
        } else if (hasDepth) {
            attachmentGLType = GL_DEPTH_ATTACHMENT;
        } else {
            attachmentGLType = GL_STENCIL_ATTACHMENT;
        }

        GL_CALL(m_GL->FramebufferTexture2D(
            GL_FRAMEBUFFER,
            attachmentGLType,
            ToGLTextureTarget(depthStencilTex.desc.type),
            depthStencilTex.id,
            0
        ));
        fb.depthStencilTexture = depthStencilTex.id;
    }

    GLenum status = m_GL->CheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        GL_CALL(m_GL->DeleteFramebuffers(1, &fb.fbo));
        GL_CALL(m_GL->BindFramebuffer(GL_FRAMEBUFFER, 0));

        return ExError{"Framebuffer incomplete (GL error)"};
    }

    GL_CALL(m_GL->BindFramebuffer(GL_FRAMEBUFFER, 0));

    bool needsResolve = false;
    bool needsDSResolve = false;
    for (size_t i{0}; i < desc.colorAttachments.size(); i++) {
        auto& tex = *m_Textures.Get(desc.colorAttachments[i]);
        if (tex.desc.samples > SampleCount::Sample1 && rp.desc.colorAttachments[i].hasResolve) {
            needsResolve = true;
            break;
        }
    }
    if (hasDepth || hasStencil) {
        auto& dsTex = *m_Textures.Get(desc.depthStencilTexture);
        if (dsTex.desc.samples > SampleCount::Sample1 && rp.desc.depthStencilAttachment.hasResolve) {
            needsDSResolve = true;
        }
    }

    // If MSAA, create resolve FBO
    if (needsResolve || needsDSResolve) {
        fb.hasResolve = true;
        GL_CALL(m_GL->GenFramebuffers(1, &fb.resolveFbo));
        GL_CALL(m_GL->BindFramebuffer(GL_FRAMEBUFFER, fb.resolveFbo));

        auto createSingleSample = [this](const TextureDesc& msaaDesc) {
            GLuint resolveTex;
            GL_CALL(m_GL->GenTextures(1, &resolveTex));
            GL_CALL(m_GL->BindTexture(GL_TEXTURE_2D, resolveTex));

            GL_CALL(m_GL->TexImage2D(
                GL_TEXTURE_2D,
                0,
                ToGLTextureInternalFormat(msaaDesc.format),
                msaaDesc.width,
                msaaDesc.height,
                0,
                ToGLTextureFormat(msaaDesc.format),
                ToGLTextureType(msaaDesc.format),
                nullptr
            ));
            GL_CALL(m_GL->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
            GL_CALL(m_GL->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
            
            GL_CALL(m_GL->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
            GL_CALL(m_GL->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

            return resolveTex;
        };

        for (uint32_t i = 0; i < desc.colorAttachments.size(); ++i) {
            auto& tex = *m_Textures.Get(desc.colorAttachments[i]);
            if (tex.desc.samples > SampleCount::Sample1 && !tex.resolveId) {
                GLuint resolveTex = createSingleSample(tex.desc);
                GL_CALL(m_GL->FramebufferTexture2D(
                    GL_FRAMEBUFFER,
                    GL_COLOR_ATTACHMENT0 + i,
                    GL_TEXTURE_2D,
                    resolveTex,
                    0
                ));
                tex.resolveId = resolveTex;
            }
        }

        // Depth/Stencil resolve
        if (needsDSResolve) {
            auto& depthTex = *m_Textures.Get(desc.depthStencilTexture);
            if (!depthTex.resolveId) {
                GLuint resolveTex = createSingleSample(depthTex.desc);
                GL_CALL(m_GL->FramebufferTexture2D(
                    GL_FRAMEBUFFER,
                    hasDepth && hasStencil ? GL_DEPTH_STENCIL_ATTACHMENT
                                        : (hasDepth ? GL_DEPTH_ATTACHMENT : GL_STENCIL_ATTACHMENT),
                    GL_TEXTURE_2D,
                    depthTex.id,
                    0
                ));
                depthTex.resolveId = resolveTex;
            }
        }

        GLenum rsstatus = m_GL->CheckFramebufferStatus(GL_FRAMEBUFFER);
        if (rsstatus != GL_FRAMEBUFFER_COMPLETE) {
            return ExError{"Resolve FBO incomplete"};
        }
    }

    fb.alive  = true;
    fb.hasDepth = hasDepth;
    fb.hasStencil = hasStencil;
    fb.width  = desc.width;
    fb.height = desc.height;
    fb.renderPass = desc.renderPass;

    return fb.External();
}

ExError GLGraphicsBackend::DestroyFramebuffer(const FramebufferHandle& handle) {
    if (m_Framebuffers.IsEqual(handle, m_DefaultBackbuffer))
        return {"Cannot delete singular default backbuffer FBO, OpenGL is implicit"};
    if (!m_Framebuffers.IsValid(handle))
        return {"Invalid Handle"};

    GLFramebuffer& fb = *m_Framebuffers.Get(handle);

    GL_CALL(m_GL->DeleteFramebuffers(1, &fb.fbo));
    fb.fbo = 0;
    m_Framebuffers.Delete(handle);
    return ExError::NoError();
}

ExResult<ShaderHandle> GLGraphicsBackend::CreateProgram(const ShaderDesc& desc) {
    bool hasCompute{false};
    bool hasOthers{false};

    for (size_t i{0}; i < desc.entryPoints.size(); i++) {
        if (desc.entryPoints[i].stage == ShaderStage::Compute) {
            hasCompute = true;
        } else {
            hasOthers = true;
        }
    }
    if (hasCompute && hasOthers) {
        return ExError{"Cannot keep fragment/vertex/etc. along with compute in a pipeline program; Compute is separate."};
    }

    auto& program = *m_Programs.Reserve();
    program.id = m_GL->CreateProgram(); 

    int major = 0, minor = 0;
    GL_CALL(m_GL->GetIntegerv(GL_MAJOR_VERSION, &major));
    GL_CALL(m_GL->GetIntegerv(GL_MINOR_VERSION, &minor));

    // TODO: Add GLES
    std::string modName = "glsl_" + std::to_string(major) + std::to_string(minor) + "0";

    slang::TargetDesc target{};
    target.format = SLANG_GLSL;
    target.profile = m_SlangGlobal->findProfile(modName.c_str());

    slang::SessionDesc sdesc{};
    sdesc.targets = &target;
    sdesc.targetCount = 1;
    sdesc.searchPaths = &desc.sourcePath; // or directory
    sdesc.searchPathCount = 1;

    Slang::ComPtr<slang::ISession> session{};
    m_SlangGlobal->createSession(sdesc, session.writeRef());

    Slang::ComPtr<slang::IBlob> errorBlob{};
    Slang::ComPtr<slang::IModule> module{
        session->loadModule(desc.sourcePath, errorBlob.writeRef())
    };

    if (!module) {
        std::stringstream error_msg{};
        std::string start = "Failed to load Slang module\n\n";
        error_msg.write(start.c_str(), start.size());
        error_msg.write((const char*)errorBlob->getBufferPointer(), errorBlob->getBufferSize());
        GL_CALL(m_GL->DeleteProgram(program.id));
        program.id = 0;
        return {error_msg.str()};
    }

    std::vector<slang::IComponentType*> components;
    components.push_back(module.get());

    std::vector<Slang::ComPtr<slang::IEntryPoint>> entryPoints;

    auto& eps = desc.entryPoints;
    for (auto& ep : eps) {
        Slang::ComPtr<slang::IEntryPoint> entry;
        module->findEntryPointByName(ep.name, entry.writeRef());

        entryPoints.push_back(entry);
        components.push_back(entry.get());
    }

    Slang::ComPtr<slang::IComponentType> linked;
    session->createCompositeComponentType(
        components.data(),
        (SlangInt) components.size(),
        linked.writeRef(),
        nullptr
    );

    for (size_t i{0}; i < desc.entryPoints.size(); ++i) {
        Slang::ComPtr<slang::IBlob> code;
        linked->getEntryPointCode(
            (SlangInt)i,
            0,
            code.writeRef(),
            nullptr
        );

        const char* glsl = (const char*) code->getBufferPointer();

        std::string glName = desc.entryPoints[i].name;
        GLenum glStage = ToGLShaderStage(desc.entryPoints[i].stage);

        GLuint shader = m_GL->CreateShader(glStage);
        GL_CALL(m_GL->ShaderSource(shader, 1, &glsl, nullptr));
        GL_CALL(m_GL->CompileShader(shader));

        GLint state;
        GL_CALL(m_GL->GetShaderiv(shader, GL_COMPILE_STATUS, &state));
        if (!state) {
            std::vector<char> log(4096);
            GLsizei length;
            GL_CALL(m_GL->GetShaderInfoLog(shader, log.size(), &length, log.data()));
    
            std::stringstream error_msg{};
            std::string start = "Failed to load GLSL entrypoint \"" + glName + "\"\n\n";
            error_msg.write(start.c_str(), start.size());
            error_msg.write(log.data(), length);
    
            GL_CALL(m_GL->DeleteShader(shader));
            GL_CALL(m_GL->DeleteProgram(program.id));
            program.id = 0;
            return {error_msg.str()};
        }

        GL_CALL(m_GL->AttachShader(program.id, shader));
        GL_CALL(m_GL->DeleteShader(shader));
    }

    GL_CALL(m_GL->LinkProgram(program.id));

    GLint linkedState;
    GL_CALL(m_GL->GetProgramiv(program.id, GL_LINK_STATUS, &linkedState));
    if (!linkedState) {
        std::vector<char> log(4096);
        GLsizei length;
        GL_CALL(m_GL->GetProgramInfoLog(program.id, log.size(), &length, log.data()));
        std::stringstream error_msg{};
        std::string start = "Failed to link GLSL shaders\n\n";
        error_msg.write(start.c_str(), start.size());
        error_msg.write(log.data(), length);
        GL_CALL(m_GL->DeleteProgram(program.id));
        program.id = 0;
        return {error_msg.str()};
    }
    program.alive = true;

    return program.External();
}

ExError GLGraphicsBackend::DestroyProgram(const ShaderHandle& handle) {
    if (!m_Programs.IsValid(handle))
        return {"Invalid Handle"};

    GLProgram& program = *m_Programs.Get(handle);

    if (m_CurrentState.program == program.id) m_CurrentState.program = 0;
    GL_CALL(m_GL->DeleteProgram(program.id));
    program.id = 0;
    m_Programs.Delete(handle);
    return ExError::NoError();
}

ExResult<RenderPipelineHandle> GLGraphicsBackend::CreateRenderPipeline(const RenderPipelineDesc& desc) {
    if (!m_Programs.IsValid(desc.shader))
        return ExError("Invalid Pipeline Shader");

    auto& pipeline = *m_RenderPipelines.Reserve();
    pipeline.desc = desc;
    pipeline.alive = true;

    return pipeline.External();
}

ExError GLGraphicsBackend::DestroyRenderPipeline(const RenderPipelineHandle& handle) {
    if (!m_RenderPipelines.IsValid(handle))
        return {"Invalid Handle"};
    auto& pipeline = *m_RenderPipelines.Get(handle);
    for (auto [vc, vao] : pipeline.vaoCache) {
        if (m_CurrentState.currentVao == vao) m_CurrentState.currentVao = 0;
        if (m_CurrentState.currentVbo == vc.vbo) m_CurrentState.currentVbo = 0;
        if (m_CurrentState.currentEbo == vc.ebo) m_CurrentState.currentEbo = 0;

        GLuint vaoCpy = vao;
        GL_CALL(m_GL->DeleteVertexArrays(1, &vaoCpy));
        GL_CALL(m_GL->DeleteBuffers(1, &vc.vbo));
        GL_CALL(m_GL->DeleteBuffers(1, &vc.ebo));
    }
    pipeline.vaoCache.clear();
    m_RenderPipelines.Delete(handle);
    return ExError::NoError();
}

ExResult<ComputePipelineHandle> GLGraphicsBackend::CreateComputePipeline(const ComputePipelineDesc& desc) {
    auto& pipeline = *m_ComputePipelines.Reserve();
    
    pipeline.desc = desc;
    pipeline.alive = true;

    return pipeline.External();
}

ExError GLGraphicsBackend::DestroyComputePipeline(const ComputePipelineHandle& handle) {
    if (!m_ComputePipelines.IsValid(handle))
        return {"Invalid Handle"};

    m_ComputePipelines.Delete(handle);
    return ExError::NoError();
}

ExResult<RenderPassHandle> GLGraphicsBackend::CreateDefaultRenderPass(const DefaultRenderPassDesc& desc) {
    auto& pass = *m_RenderPasses.Reserve();
    RenderPassDesc rpdesc{};
    AttachmentDesc colorDesc{};

    colorDesc.hasResolve = false;
    colorDesc.samples = SampleCount::Sample1;
    colorDesc.passOps = desc.colorOps;
    colorDesc.format = m_SurfaceInfo.colorFormat;

    rpdesc.colorAttachments = {{ colorDesc }};
    rpdesc.hasDepth = m_SurfaceInfo.depthBits > 0;
    rpdesc.hasStencil = m_SurfaceInfo.stencilBits > 0;
    if (rpdesc.hasDepth || rpdesc.hasStencil) {
        rpdesc.depthStencilAttachment.format = m_SurfaceInfo.depthStencilFormat;
        rpdesc.depthStencilAttachment.passOps = desc.depthStencilOps;
        rpdesc.depthStencilAttachment.samples = SampleCount::Sample1;
        rpdesc.depthStencilAttachment.hasResolve = false;
    }    
    pass.desc = rpdesc;
    pass.alive = true;

    return pass.External();
}

ExResult<RenderPassHandle> GLGraphicsBackend::CreateRenderPass(const RenderPassDesc& desc) {
    auto& pass = *m_RenderPasses.Reserve();
    pass.desc = desc;
    pass.alive = true;
    return pass.External();
}

ExError GLGraphicsBackend::DestroyRenderPass(const RenderPassHandle& handle) {
    if (!m_RenderPasses.IsValid(handle))
        return {"Invalid Handle"};

    m_RenderPasses.Delete(handle);
    return ExError::NoError();
}

ExError GLGraphicsBackend::Dispatch(ICommandList& cmd, uint32_t x, uint32_t y, uint32_t z) {
    if (x > m_Capabilities.maxWorkGroupCount[0] ||
        y > m_Capabilities.maxWorkGroupCount[1] ||
        z > m_Capabilities.maxWorkGroupCount[2]
    ) return ExError{"Dispatch exceeds max workGroup count"};

    if (!m_CurrentComputePipeline.bound) {
        return ExError{"Bad commands, requested Compute Dispatch, but no Compute Pipeline bound!"};
    }
    Execute(static_cast<GLCommandList&>(cmd));
    GL_CALL(m_GL->DispatchCompute(x, y, z));
    return ExError::NoError();
}

ExError GLGraphicsBackend::Barrier(ICommandList&, std::vector<ResourceTransition> transitions) {
    GLenum bits = 0;

    for (size_t i{0}; i < transitions.size(); i++) {
        bits |= ToGLBarrierBit(transitions[i].newState);
    }
    if (bits != 0) {
        GL_CALL(m_GL->MemoryBarrier(bits));
    }
    return ExError::NoError();
}

ExResult<GLuint> GLGraphicsBackend::CreateVertexArray(GLRenderPipeline& pipeline) {
    GLuint vao{};

    GL_CALL(m_GL->GenVertexArrays(1, &vao));
    GL_CALL(m_GL->BindVertexArray(vao));
    GL_CALL(m_GL->BindBuffer(GL_ARRAY_BUFFER, m_CurrentState.currentVbo));
    if (m_CurrentState.currentEbo > 0) {
        GL_CALL(m_GL->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_CurrentState.currentEbo));
    }

    auto& desc = pipeline.desc;
    auto& attrSpan = desc.layout.attributes;
    for (size_t i{0}; i < attrSpan.size(); i++) {
        auto& attr = attrSpan[i];
        GL_CALL(m_GL->EnableVertexAttribArray(attr.location));

        switch (attr.typeDesc._class) {
            case VertexAttributeClass::Float: {
                GL_CALL(m_GL->VertexAttribPointer(
                    attr.location,
                    attr.componentCount,
                    ToGLVertexAttribType(attr.typeDesc.type),
                    attr.normalized,
                    desc.layout.stride,
                    (void*)(uintptr_t)attr.offset
                ));
                break;
            }
            case VertexAttributeClass::Int: {
                GL_CALL(m_GL->VertexAttribIPointer(
                    attr.location,
                    attr.componentCount,
                    ToGLVertexAttribType(attr.typeDesc.type),
                    desc.layout.stride,
                    (void*)(uintptr_t)attr.offset
                ));
                break;
            }
            case VertexAttributeClass::Double: {
                if (!m_Capabilities.Has(GraphicsCapEnum::LongPointers) || m_GL->VertexAttribLPointer == nullptr) {
                    return ExError("Invalid vertex attribute type: Host GPU doesn't support 64 bit pointers");
                }
                GL_CALL(m_GL->VertexAttribLPointer(
                    attr.location,
                    attr.componentCount,
                    ToGLVertexAttribType(attr.typeDesc.type),
                    desc.layout.stride,
                    (void*)(uintptr_t)attr.offset
                ));
                break;
            }
        }
        if (attr.divisor > 0)
            GL_CALL(m_GL->VertexAttribDivisor(attr.location, attr.divisor));
    }
    GL_CALL(m_GL->BindVertexArray(0));

    return vao;
}

ExError GLGraphicsBackend::PrepVertexArray(GLRenderPipeline& pipeline) {
    if (!pipeline.alive) {
        return ExError{"PrepVertexArray() Failed: Dead/Invalid pipeline"};
    }
    if (m_CurrentState.currentVbo == 0) {
        return ExError{"PrepVertexArray() Failed: No VBO bound"};
    }

    VAOKey key{m_CurrentState.currentVbo, m_CurrentState.currentEbo};
    auto it = pipeline.vaoCache.find(key);
    if (it != pipeline.vaoCache.end()) {
        auto& vao = it->second;
        m_CurrentState.currentVao = vao;
        GL_CALL(m_GL->BindVertexArray(vao));
        return ExError::NoError();
    }

    auto res = CreateVertexArray(pipeline);
    if (!res.has_value()) {
        return res.error();
    }
    auto vao = res.value();

    pipeline.vaoCache[key] = vao;
    GL_CALL(m_GL->BindVertexArray(vao));

    return ExError::NoError();
}

ExError GLGraphicsBackend::InternalExecute(CommandType type, data::BufferDataStream& args) {
    if (type >= CommandType::BindVertexBuffer) {
        if (!m_CurrentRenderPass.bound)
            return {"GLCommand::Execute \"GLCommandType::BindVertices/DrawXYZ\" Failed: No RenderPass bound, use BeginRenderPass()"};
        if (!m_CurrentRenderPipeline.bound)
            return {"GLCommand::Execute \"GLCommandType::BindVertices/DrawXYZ\" Failed: No RenderPipeline bound, use BindRenderPipeline()"};

        if (!m_RenderPasses.IsValid(m_CurrentRenderPass.handle))
            return {"GLCommand::Execute \"GLCommandType::BindVertices/DrawXYZ\" Failed: Invalid RenderPass bound!"};
        if (!m_RenderPipelines.IsValid(m_CurrentRenderPipeline.handle))
            return {"GLCommand::Execute \"GLCommandType::BindVertices/DrawXYZ\" Failed: Invalid RenderPipeline bound!"};
    }

    switch (type) {
        case CommandType::SetViewport: {
            CommandSetViewport cmd;
            AX_PROPAGATE_RESULT_ERROR(args.Read(&cmd, sizeof(cmd)));
            GL_CALL(m_GL->Viewport(cmd.x, cmd.y, cmd.width, cmd.height));
            GL_CALL(m_GL->DepthRange(cmd.minDepth, cmd.maxDepth));
            break;
        };
        case CommandType::SetScissor: {
            CommandSetScissor cmd;
            AX_PROPAGATE_RESULT_ERROR(args.Read(&cmd, sizeof(cmd)));
            GL_CALL(m_GL->Scissor(cmd.x, cmd.y, cmd.width, cmd.height));
            break;
        }
        case CommandType::BeginRenderPass: {
            CommandBeginRenderPass cmd;
            AX_PROPAGATE_RESULT_ERROR(args.Read(&cmd, sizeof(cmd)));

            if (!m_RenderPasses.IsValid(cmd.pass))
                return {"GLCommand::Execute \"BeginRenderPass\" Failed: Invalid RenderPass handle"};
            if (!m_Framebuffers.IsValid(cmd.framebuffer))
                return {"GLCommand::Execute \"BeginRenderPass\" Failed: Invalid Framebuffer handle"};

            GLRenderPass& pass = *m_RenderPasses.Get(cmd.pass);
            GLFramebuffer& fb  = *m_Framebuffers.Get(cmd.framebuffer);

            GL_CALL(m_GL->BindFramebuffer(GL_FRAMEBUFFER, fb.fbo));

            std::vector<GLenum> drawBuffers(fb.fbo != 0 ?
                pass.desc.colorAttachments.size() :
                std::max(pass.desc.colorAttachments.size(), (std::size_t)1)
            );

            if (fb.fbo !=  0) {
                if (drawBuffers.size() > m_Capabilities.maxColorAttachments) {
                    return {"GLCommand::Execute \"BeginRenderPass\" Failed: Maximum MRT supported by device is " + std::to_string(m_Capabilities.maxColorAttachments)};
                }
                for (size_t i = 0; i < pass.desc.colorAttachments.size(); ++i)
                    drawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;

                GL_CALL(m_GL->DrawBuffers((GLsizei)drawBuffers.size(), drawBuffers.data()));    
            } else if (!drawBuffers.empty()) {
                GL_CALL(m_GL->DrawBuffer(GL_BACK));
            } else {
                GL_CALL(m_GL->DrawBuffer(GL_NONE));
            }

            // Clear attachments based on LoadOp
            for (size_t i = 0; i < pass.desc.colorAttachments.size(); ++i) {
                auto& att = pass.desc.colorAttachments[i];

                if (att.passOps.load == LoadOp::Clear) {
                    auto colors = cmd.clear.clearColor;
                    GL_CALL(m_GL->ClearBufferfv(GL_COLOR, (GLint)i, colors));
                }
            }
            if (fb.hasDepth) {
                auto& depthStencil = pass.desc.depthStencilAttachment;

                if (depthStencil.passOps.load == LoadOp::Clear) {
                    GL_CALL(m_GL->ClearBufferfv(GL_DEPTH, 0, &cmd.clear.clearDepth));
                }
            }
            if (fb.hasStencil) {
                auto& depthStencil = pass.desc.depthStencilAttachment;

                if (depthStencil.passOps.load == LoadOp::Clear) {
                    GL_CALL(m_GL->ClearBufferiv(GL_STENCIL, 0, (const GLint*)&cmd.clear.clearStencil));
                }
            }

            pass.fbInUse.Bind(cmd.framebuffer);
            m_CurrentRenderPass.Bind(cmd.pass);
            break;
        }
        case CommandType::EndRenderPass: {
            CommandEndRenderPass cmd;
            AX_PROPAGATE_RESULT_ERROR(args.Read(&cmd, sizeof(cmd)));

            if (!m_RenderPasses.IsValid(cmd.pass))
                return {"GLCommand::Execute \"GLCommandType::EndRenderPass\" Failed: Invalid RenderPass handle"};

            GLRenderPass& pass = *m_RenderPasses.Get(cmd.pass);
            GLFramebuffer& fb = *m_Framebuffers.Get(pass.fbInUse.handle);

            if (fb.hasResolve) {
                GL_CALL(m_GL->BindFramebuffer(GL_READ_FRAMEBUFFER, fb.fbo));
                GL_CALL(m_GL->BindFramebuffer(GL_DRAW_FRAMEBUFFER, fb.resolveFbo));

                for (size_t i = 0; i < pass.desc.colorAttachments.size(); ++i) {
                    GL_CALL(m_GL->ReadBuffer(GL_COLOR_ATTACHMENT0 + i));
                    GL_CALL(m_GL->DrawBuffer(GL_COLOR_ATTACHMENT0 + i));

                    GL_CALL(m_GL->BlitFramebuffer(
                        0, 0, fb.width, fb.height,
                        0, 0, fb.width, fb.height,
                        GL_COLOR_BUFFER_BIT,
                        GL_NEAREST
                    ));
                }
                // Rebind original framebuffer
                GL_CALL(m_GL->BindFramebuffer(GL_FRAMEBUFFER, fb.fbo));
            }

            if (fb.fbo !=  0) {
                std::vector<GLenum> invalidateAttachments;

                for (size_t i = 0; i < pass.desc.colorAttachments.size(); ++i) {
                    if (pass.desc.colorAttachments[i].passOps.store == StoreOp::Discard) {
                        if (fb.fbo != 0) {
                            invalidateAttachments.push_back(GL_COLOR_ATTACHMENT0 + i);
                        } else {
                            invalidateAttachments.push_back(GL_COLOR);
                            break;
                        }
                    }
                }

                if (pass.desc.depthStencilAttachment.passOps.store == StoreOp::Discard) {
                    if (pass.desc.hasDepth && pass.desc.hasStencil) {
                        invalidateAttachments.push_back(fb.fbo != 0 ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_STENCIL);
                    } else if (pass.desc.hasDepth) {
                        invalidateAttachments.push_back(fb.fbo != 0 ? GL_DEPTH_ATTACHMENT : GL_DEPTH);
                    } else if (pass.desc.hasStencil) {
                        invalidateAttachments.push_back(fb.fbo != 0 ? GL_STENCIL_ATTACHMENT : GL_STENCIL);
                    }
                }

                if (!invalidateAttachments.empty()) {
                    GL_CALL(m_GL->InvalidateFramebuffer(
                        GL_FRAMEBUFFER,
                        (GLsizei)invalidateAttachments.size(),
                        invalidateAttachments.data()
                    ));
                }
            }

            GL_CALL(m_GL->BindFramebuffer(GL_FRAMEBUFFER, 0));

            pass.fbInUse.UnBind();
            m_CurrentRenderPass.UnBind();

            m_CurrentState.program = 0;
            m_CurrentState.currentVao = 0;
            m_CurrentState.currentVbo = 0;
            m_CurrentState.currentEbo = 0;
            break;
        }
        case CommandType::BindRenderPipeline: {
            CommandBindRenderPipeline cmd;
            AX_PROPAGATE_RESULT_ERROR(args.Read(&cmd, sizeof(cmd)));

            if (!m_RenderPipelines.IsValid(cmd.handle))
                return {"GLCommand::Execute \"GLCommandType::BindRenderPipeline\" Failed: Invalid pipeline handle"};

            if (!m_CurrentRenderPass.bound)
                return {"GLCommand::Execute \"GLCommandType::BindRenderPipeline\" Failed: No RenderPass bound, use BeginRenderPass()"};
            if (!m_RenderPasses.IsValid(m_CurrentRenderPass.handle))
                return {"GLCommand::Execute \"GLCommandType::BindRenderPipeline\" Failed: Invalid RenderPass bound!"};

            auto& pipeline = *m_RenderPipelines.Get(cmd.handle);

            if (!m_Programs.IsValid(pipeline.desc.shader))
                return {"GLCommand::Execute \"GLCommandType::BindRenderPipeline\" Failed: Invalid Pipeline shader program!"};

            auto& desc = pipeline.desc;

            if (
                m_CurrentRenderPipeline.bound &&
                m_CurrentRenderPipeline.handle.index == cmd.handle.index &&
                m_CurrentRenderPipeline.handle.generation == cmd.handle.generation
            ) {
                break;
            }

            m_CurrentRenderPipeline.Bind(cmd.handle);

            auto& program = *m_Programs.Get(pipeline.desc.shader);
            if (m_CurrentState.program != program.id) {
                GL_CALL(m_GL->UseProgram(program.id));
                m_CurrentState.program = program.id;
            }

            bool wantCull = desc.raster.cull != CullMode::None;

            if (m_CurrentState.cullEnabled != wantCull) {
                m_CurrentState.cullEnabled = wantCull;
                wantCull ? m_GL->Enable(GL_CULL_FACE)
                        : m_GL->Disable(GL_CULL_FACE);
            }

            if (wantCull) {
                GLenum face = desc.raster.cull == CullMode::Front ? GL_FRONT : GL_BACK;

                if (m_CurrentState.cullFace != face) {
                    GL_CALL(m_GL->CullFace(face));
                    m_CurrentState.cullFace = face;
                }
            }

            GLenum front = desc.raster.frontFace == FrontFace::Clockwise ? GL_CW : GL_CCW;

            if (m_CurrentState.frontFace != front) {
                GL_CALL(m_GL->FrontFace(front));
                m_CurrentState.frontFace = front;
            }

            GLenum poly = desc.raster.fill == FillMode::Wireframe ? GL_LINE : GL_FILL;

            if (m_CurrentState.polygonMode != poly) {
                GL_CALL(m_GL->PolygonMode(GL_FRONT_AND_BACK, poly));
                m_CurrentState.polygonMode = poly;
            }

            if (m_CurrentState.depthTest != desc.depth.depthTest) {
                m_CurrentState.depthTest = desc.depth.depthTest;
                desc.depth.depthTest ? m_GL->Enable(GL_DEPTH_TEST)
                                    : m_GL->Disable(GL_DEPTH_TEST);
            }

            if (m_CurrentState.depthWrite != desc.depth.depthWrite) {
                GL_CALL(m_GL->DepthMask(desc.depth.depthWrite ? GL_TRUE : GL_FALSE));
                m_CurrentState.depthWrite = desc.depth.depthWrite;
            }

            GLenum depthFunc = ToGLCompare(desc.depth.depthCompare);
            if (m_CurrentState.depthFunc != depthFunc) {
                GL_CALL(m_GL->DepthFunc(depthFunc));
                m_CurrentState.depthFunc = depthFunc;
            }

            if (m_CurrentState.stencilTest != desc.depth.stencilTest) {
                m_CurrentState.stencilTest = desc.depth.stencilTest;
                desc.depth.stencilTest ? m_GL->Enable(GL_STENCIL_TEST)
                                    : m_GL->Disable(GL_STENCIL_TEST);
            }

            if (desc.depth.stencilTest) {
                GL_CALL(m_GL->StencilFuncSeparate(
                    GL_FRONT,
                    ToGLCompare(desc.depth.stencilFront.compare),
                    desc.depth.stencilFront.reference,
                    desc.depth.stencilFront.compareMask));

                GL_CALL(m_GL->StencilOpSeparate(
                    GL_FRONT,
                    ToGLStencilOp(desc.depth.stencilFront.failOp),
                    ToGLStencilOp(desc.depth.stencilFront.depthFailOp),
                    ToGLStencilOp(desc.depth.stencilFront.passOp)));

                GL_CALL(m_GL->StencilMaskSeparate(
                    GL_FRONT,
                    desc.depth.stencilFront.writeMask));

                GL_CALL(m_GL->StencilFuncSeparate(
                    GL_BACK,
                    ToGLCompare(desc.depth.stencilBack.compare),
                    desc.depth.stencilBack.reference,
                    desc.depth.stencilBack.compareMask));

                GL_CALL(m_GL->StencilOpSeparate(
                    GL_BACK,
                    ToGLStencilOp(desc.depth.stencilBack.failOp),
                    ToGLStencilOp(desc.depth.stencilBack.depthFailOp),
                    ToGLStencilOp(desc.depth.stencilBack.passOp)));

                GL_CALL(m_GL->StencilMaskSeparate(
                    GL_BACK,
                    desc.depth.stencilBack.writeMask));
            }

            if (m_CurrentState.blendEnabled != desc.blend.enabled) {
                m_CurrentState.blendEnabled = desc.blend.enabled;
                desc.blend.enabled ? m_GL->Enable(GL_BLEND)
                                : m_GL->Disable(GL_BLEND);
            }

            if (desc.blend.enabled) {
                GLenum srcC = ToGLBlendFactor(desc.blend.srcColor);
                GLenum dstC = ToGLBlendFactor(desc.blend.dstColor);
                GLenum srcA = ToGLBlendFactor(desc.blend.srcAlpha);
                GLenum dstA = ToGLBlendFactor(desc.blend.dstAlpha);
                GLenum opC  = ToGLBlendOp(desc.blend.colorOp);
                GLenum opA  = ToGLBlendOp(desc.blend.alphaOp);

                if (
                    m_CurrentState.srcColor != srcC ||
                    m_CurrentState.dstColor != dstC ||
                    m_CurrentState.srcAlpha != srcA ||
                    m_CurrentState.dstAlpha != dstA
                ) {
                    GL_CALL(m_GL->BlendFuncSeparate(srcC, dstC, srcA, dstA));
                    m_CurrentState.srcColor = srcC;
                    m_CurrentState.dstColor = dstC;
                    m_CurrentState.srcAlpha = srcA;
                    m_CurrentState.dstAlpha = dstA;
                }

                if (
                    m_CurrentState.blendColorOp != opC ||
                    m_CurrentState.blendAlphaOp != opA
                ) {
                    GL_CALL(m_GL->BlendEquationSeparate(opC, opA));
                    m_CurrentState.blendColorOp = opC;
                    m_CurrentState.blendAlphaOp = opA;
                }
            }
            break;
        }
        case CommandType::BindComputePipeline: {
            CommandBindComputePipeline cmd;
            AX_PROPAGATE_RESULT_ERROR(args.Read(&cmd, sizeof(cmd)));

            if (!m_ComputePipelines.IsValid(cmd.handle))
                return {"GLCommand::Execute \"GLCommandType::BindComputePipeline\" Failed: Invalid compute-pipeline handle"};

            m_CurrentComputePipeline.Bind(cmd.handle);
            break;
        }
        case CommandType::BindVertexBuffer: {
            CommandBindVertexBuffer cmd;
            AX_PROPAGATE_RESULT_ERROR(args.Read(&cmd, sizeof(cmd)));

            if (!m_Buffers.IsValid(cmd.handle))
                return {"GLCommand::Execute \"GLCommandType::BindVertexBuffer\" Failed: Invalid buffer handle"};

            auto& buff = *m_Buffers.Get(cmd.handle);
            auto& pipeline = *m_RenderPipelines.Get(m_CurrentRenderPipeline.handle);

            if (buff.usage != BufferUsage::Vertex) {
                return {"GLCommand::Execute \"GLCommandType::BindVertexBuffer\" Failed: Illegal buffer handle; must be VertexBuffer"};
            }
            m_CurrentState.currentVbo = buff.id;
            break;
        }
        case CommandType::BindIndexBuffer: {
            CommandBindIndexBuffer cmd;
            AX_PROPAGATE_RESULT_ERROR(args.Read(&cmd, sizeof(cmd)));

            if (!m_Buffers.IsValid(cmd.handle))
                return {"GLCommand::Execute \"GLCommandType::BindIndexBuffer\" Failed: Invalid buffer handle"};

            auto& buff = *m_Buffers.Get(cmd.handle);
            auto& pipeline = *m_RenderPipelines.Get(m_CurrentRenderPipeline.handle);

            if (buff.usage != BufferUsage::Index) {
                return {"GLCommand::Execute \"GLCommandType::BindIndexBuffer\" Failed: Illegal buffer handle; must be IndexBuffer"};
            }
            m_CurrentState.currentEbo = buff.id;
            break;
        }
        case CommandType::BindIndirectBuffer: {
            CommandBindIndirectBuffer cmd;
            AX_PROPAGATE_RESULT_ERROR(args.Read(&cmd, sizeof(cmd)));

            if (!m_Capabilities.Has(GraphicsCapEnum::IndirectDraw))
                return {"GLCommand::Execute \"GLCommandType::BindIndirectBuffer\" Failed: Indirect Rendering is Unsupported on this device"};

            if (!m_Buffers.IsValid(cmd.handle))
                return {"GLCommand::Execute \"GLCommandType::BindIndirectBuffer\" Failed: Invalid buffer handle"};

            auto& buff = *m_Buffers.Get(cmd.handle);
            if (buff.usage != BufferUsage::Indirect)
                return {"GLCommand::Execute \"GLCommandType::BindIndirectBuffer\" Failed: Illegal buffer handle: must be IndirectBuffer"};

            GL_CALL(m_GL->BindBuffer(GL_DRAW_INDIRECT_BUFFER, buff.id));
            break;
        }
        case CommandType::BindResourceSet: {
            CommandBindResourceSet cmd;
            AX_PROPAGATE_RESULT_ERROR(args.Read(&cmd, sizeof(cmd)));

            if (!m_ResourceSets.IsValid(cmd.handle))
                return {"GLCommand::Execute \"GLCommandType::BindResourceSet\" Failed: Invalid ResourceSet handle"};

            auto& res = *m_ResourceSets.Get(cmd.handle);

            for (auto& b : res.bindings) {
                switch (b.type) {
                    case BindingType::UniformBuffer: {
                        if (!m_Buffers.IsValid(b.resource.index, b.resource.generation))
                            return {"GLCommand::Execute \"GLCommandType::BindResourceSet::UniformBuffer\" Failed: Invalid Buffer handle"};

                        GL_CALL(m_GL->BindBufferRange(
                            GL_UNIFORM_BUFFER,
                            b.slot,
                            m_Buffers.Get(b.resource.AsBuffer())->id,
                            b.offset,
                            b.range
                        ));
                        break;
                    }
                    case BindingType::StorageBuffer: {
                        if (!m_Buffers.IsValid(b.resource.index, b.resource.generation))
                            return {"GLCommand::Execute \"GLCommandType::BindResourceSet::StorageBuffer\" Failed: Invalid Buffer handle"};

                        GL_CALL(m_GL->BindBufferRange(
                            GL_SHADER_STORAGE_BUFFER,
                            b.slot,
                            m_Buffers.Get(b.resource.AsBuffer())->id,
                            b.offset,
                            b.range
                        ));
                        break;
                    }
                    case BindingType::SampledTexture: {
                        if (!m_Textures.IsValid(b.resource.index, b.resource.generation))
                            return {"GLCommand::Execute \"GLCommandType::BindResourceSet::SampledTexture\" Failed: Invalid Texture handle"};
                        
                        GL_CALL(m_GL->ActiveTexture(GL_TEXTURE0 + b.slot));
                        GL_CALL(m_GL->BindTexture(GL_TEXTURE_2D, m_Textures.Get(b.resource.AsTexture())->id));
                        break;
                    }
                    case BindingType::StorageTexture: {
                        if (!m_Textures.IsValid(b.resource.index, b.resource.generation))
                            return {"GLCommand::Execute \"GLCommandType::BindResourceSet::StorageTexture\" Failed: Invalid Texture handle"};

                        GL_CALL(m_GL->BindImageTexture(
                            b.slot,
                            m_Textures.Get(b.resource.AsTexture())->id,
                            0,
                            GL_FALSE,
                            0,
                            GL_READ_WRITE,
                            ToGLTextureFormat(m_Textures.Get(b.resource.AsTexture())->desc.format)
                        ));
                        break;
                    }
                }
            }
            break;
        }
        case CommandType::Draw: {
            CommandDraw cmd;
            AX_PROPAGATE_RESULT_ERROR(args.Read(&cmd, sizeof(cmd)));

            auto& pipeline = *m_RenderPipelines.Get(m_CurrentRenderPipeline.handle);
            AX_PROPAGATE_ERROR(PrepVertexArray(pipeline));
            GL_CALL(m_GL->DrawArrays(
                ToGLPolyMode(pipeline.desc.raster.polyMode),
                cmd.firstVertex,
                cmd.vertexCount
            ));
            break;
        }
        case CommandType::DrawIndexed: {
            CommandDrawIndexed cmd;
            AX_PROPAGATE_RESULT_ERROR(args.Read(&cmd, sizeof(cmd)));

            auto& pipeline = *m_RenderPipelines.Get(m_CurrentRenderPipeline.handle);
            AX_PROPAGATE_ERROR(PrepVertexArray(pipeline));
            GL_CALL(m_GL->DrawElements(
                ToGLPolyMode(pipeline.desc.raster.polyMode),
                cmd.indexCount,
                GL_UNSIGNED_INT,
                (void*)(cmd.firstIndex * sizeof(uint32_t))
            ));
            break;
        }
        case CommandType::DrawInstanced: {
            CommandDrawInstanced cmd;
            AX_PROPAGATE_RESULT_ERROR(args.Read(&cmd, sizeof(cmd)));

            auto& pipeline = *m_RenderPipelines.Get(m_CurrentRenderPipeline.handle);
            AX_PROPAGATE_ERROR(PrepVertexArray(pipeline));
            GL_CALL(m_GL->DrawArraysInstanced(
                ToGLPolyMode(pipeline.desc.raster.polyMode),
                cmd.firstVertex,
                cmd.vertexCount,
                cmd.instanceCount
            ));
            break;
        }
        case CommandType::DrawIndexedInstanced: {
            CommandDrawIndexedInstanced cmd;
            args.Read(&cmd, sizeof(cmd));

            auto& pipeline = *m_RenderPipelines.Get(m_CurrentRenderPipeline.handle);
            AX_PROPAGATE_ERROR(PrepVertexArray(pipeline));
            GL_CALL(m_GL->DrawElementsInstanced(
                ToGLPolyMode(pipeline.desc.raster.polyMode),
                cmd.indexCount,
                GL_UNSIGNED_INT,
                (void*)cmd.firstIndex,
                cmd.instanceCount
            ));
            break;
        }
        case CommandType::DrawIndirect: {
            CommandDrawIndirect cmd;
            AX_PROPAGATE_RESULT_ERROR(args.Read(&cmd, sizeof(cmd)));

            if (!m_Capabilities.Has(GraphicsCapEnum::IndirectDraw))
                return {"GLCommand::Execute \"GLCommandType::DrawIndirect\" Failed: Indirect Rendering is Unsupported on this device"};

            auto& pipeline = *m_RenderPipelines.Get(m_CurrentRenderPipeline.handle);
            uint32_t drawCount = cmd.count;

            if (drawCount > 1) {
                if (!m_Capabilities.Has(GraphicsCapEnum::MultiDrawIndirect))
                    return {"GLCommand::Execute \"GLCommandType::DrawIndirect\" Failed: Multi-Indirect Rendering is Unsupported on this device"};

                AX_PROPAGATE_ERROR(PrepVertexArray(pipeline));
                GL_CALL(m_GL->MultiDrawArraysIndirect(
                    ToGLPolyMode(pipeline.desc.raster.polyMode),
                    (void*)cmd.offset,
                    drawCount,
                    cmd.stride
                ));
            } else {
                AX_PROPAGATE_ERROR(PrepVertexArray(pipeline));
                GL_CALL(m_GL->DrawArraysIndirect(
                    ToGLPolyMode(pipeline.desc.raster.polyMode),
                    (void*)cmd.offset
                ));
            }
            break;
        }
        case CommandType::DrawIndirectIndexed: {
            CommandDrawIndirectIndexed cmd;
            AX_PROPAGATE_RESULT_ERROR(args.Read(&cmd, sizeof(cmd)));

            if (!m_Capabilities.Has(GraphicsCapEnum::IndirectDraw))
                return {"GLCommand::Execute \"GLCommandType::DrawIndirectIndexed\" Failed: Indirect Rendering is Unsupported on this device"};

            auto& pipeline = *m_RenderPipelines.Get(m_CurrentRenderPipeline.handle);
            uint32_t drawCount = cmd.count;

            if (drawCount > 1) {
                if (!m_Capabilities.Has(GraphicsCapEnum::MultiDrawIndirect))
                    return {"GLCommand::Execute \"GLCommandType::DrawIndirectIndexed\" Failed: Multi-Indirect Rendering is Unsupported on this device"};

                AX_PROPAGATE_ERROR(PrepVertexArray(pipeline));
                GL_CALL(m_GL->MultiDrawElementsIndirect(
                    ToGLPolyMode(pipeline.desc.raster.polyMode),
                    GL_UNSIGNED_INT,
                    (void*)(cmd.firstIndex * sizeof(uint32_t)),
                    drawCount,
                    cmd.stride
                ));
            } else {
                AX_PROPAGATE_ERROR(PrepVertexArray(pipeline));
                GL_CALL(m_GL->DrawElementsIndirect(
                    ToGLPolyMode(pipeline.desc.raster.polyMode),
                    GL_UNSIGNED_INT,
                    (void*)(cmd.firstIndex * sizeof(uint32_t))
                ));
            }
            break;
        }
    }
    return ExError::NoError();
}

ExError GLGraphicsBackend::Execute(ICommandList& cmd) {
    auto& glCmd = static_cast<GLCommandList&>(cmd);

    auto cmdGuard = glCmd.CommandGuard();
    auto& cmdBuffer = *cmdGuard.m_Buffer.get();

    CommandType lastType{0};
    uint16_t lastSec{0};

    AX_PROPAGATE_RESULT_ERROR(cmdBuffer.Read(&lastSec, 2));

    if (lastSec != CMDL_BEGIN) {
        return {"GLCommand::Execute Failed: CommandList pre section doesn't match CMDL_BEGIN"};
    }
    while (lastSec != CMDL_END) {
        if (cmdBuffer.EndOfStream()) {
            return {"GLCommand::Execute Failed: Unexpected EndOfStream"};
        }

        AX_PROPAGATE_RESULT_ERROR(cmdBuffer.Read(&lastSec, 2));
        if (lastSec == CMDL_END) {
            break;
        } else  if (lastSec != CMD_HEADER) {
            return {"GLCommand::Execute Failed: Command pre section doesn't match CMD_HEADER"};
        }

        AX_PROPAGATE_RESULT_ERROR(cmdBuffer.Read(&lastType, 4));
        AX_PROPAGATE_ERROR(InternalExecute(lastType, cmdBuffer));

        AX_PROPAGATE_RESULT_ERROR(cmdBuffer.Read(&lastSec, 2));
        if (lastSec != CMD_FOOTER) {
            return {"GLCommand::Execute Failed: Command post section doesn't match CMD_FOOTER"};
        }
    }
    return ExError::NoError();
}

utils::ExResult<uint32_t> GLGraphicsBackend::AcquireNextImage() {
    return 0; // OpenGL always has single backbuffer
}

utils::ExResult<FramebufferHandle> GLGraphicsBackend::GetSwapchainFramebuffer(uint32_t imageIndex) {
    if (imageIndex != 0)
        return ExError{"OpenGL has only one single backbuffer and is (FBO 0)"};
    return m_DefaultBackbuffer;
}

utils::ExError GLGraphicsBackend::Present(uint32_t imageIndex) {
    if (imageIndex != 0)
        return ExError{"OpenGL has only one backbuffer"};

    m_Context->SwapBuffers();
    return ExError::NoError();
}

utils::ExResult<ResourceSetHandle> GLGraphicsBackend::CreateResourceSet(const ResourceSetDesc& desc) {
    auto& handle = *m_ResourceSets.Reserve();

    handle.bindings.resize(desc.bindings.size());
    for (size_t i{0}; i < desc.bindings.size(); i++) {
        handle.bindings[i] = desc.bindings[i];
    }
    handle.layoutID = desc.layoutID;
    handle.version = 1;
    handle.alive = true;

    return handle.External();
}

utils::ExError GLGraphicsBackend::UpdateResourceSet(const ResourceSetHandle& handle, std::vector<Binding> bindings) {
    if (!m_ResourceSets.IsValid(handle))
        return {"Invalid handle"};
    auto& res = *m_ResourceSets.Get(handle);

    res.bindings.resize(bindings.size());
    for (size_t i{0}; i < bindings.size(); i++) {
        res.bindings[i] = bindings[i];
    }
    res.version++;
    return ExError::NoError();
}

utils::ExError GLGraphicsBackend::DestroyResourceSet(const ResourceSetHandle& handle) {
    if (!m_ResourceSets.IsValid(handle))
        return {"Invalid handle"};

    m_ResourceSets.Delete(handle);
    return ExError::NoError();
}

GLenum ToGLTextureWrap(TextureWrap wrap) {
    switch (wrap) {
        case TextureWrap::Repeat:         return GL_REPEAT;
        case TextureWrap::MirrorRepeat:   return GL_MIRRORED_REPEAT;
        case TextureWrap::ClampToEdge:    return GL_CLAMP_TO_EDGE;
        case TextureWrap::ClampToBorder:  return GL_CLAMP_TO_BORDER;
        default: return GL_NONE;
    }
}

GLenum ToGLTextureFilter(TextureFilter filter, bool mipmapEnabled) {
    switch (filter) {
        case TextureFilter::Nearest:
            return mipmapEnabled ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST;
        case TextureFilter::Linear:
            return mipmapEnabled ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
        default: return GL_NONE;
    }
}

GLenum ToGLTextureInternalFormat(TextureFormat fmt) { // alan barmigardim
    switch (fmt) {
        // --- 8-bit ---
        case TextureFormat::R8_UNORM:      return GL_R8;
        case TextureFormat::R8_SNORM:      return GL_R8_SNORM;
        case TextureFormat::R8_UINT:       return GL_R8UI;
        case TextureFormat::R8_SINT:       return GL_R8I;

        case TextureFormat::RG8_UNORM:     return GL_RG8;
        case TextureFormat::RG8_SNORM:     return GL_RG8_SNORM;
        case TextureFormat::RG8_UINT:      return GL_RG8UI;
        case TextureFormat::RG8_SINT:      return GL_RG8I;

        case TextureFormat::RGB8_UNORM:     return GL_RGB8;
        case TextureFormat::RGB8_SNORM:     return GL_RGB8_SNORM;
        case TextureFormat::RGB8_UINT:      return GL_RGB8UI;
        case TextureFormat::RGB8_SINT:      return GL_RGB8I;

        case TextureFormat::RGBA8_UNORM:   return GL_RGBA8;
        case TextureFormat::RGBA8_SNORM:   return GL_RGBA8_SNORM;
        case TextureFormat::RGBA8_UINT:    return GL_RGBA8UI;
        case TextureFormat::RGBA8_SINT:    return GL_RGBA8I;

        case TextureFormat::BGRA8_UNORM:   return GL_RGBA8;
        case TextureFormat::RGBA8_SRGB:    return GL_SRGB8_ALPHA8;

        // --- 16-bit ---
        case TextureFormat::R16_UNORM:     return GL_R16;
        case TextureFormat::R16_SNORM:     return GL_R16_SNORM;
        case TextureFormat::R16_UINT:      return GL_R16UI;
        case TextureFormat::R16_SINT:      return GL_R16I;
        case TextureFormat::R16_FLOAT:     return GL_R16F;

        case TextureFormat::RG16_UNORM:    return GL_RG16;
        case TextureFormat::RG16_SNORM:    return GL_RG16_SNORM;
        case TextureFormat::RG16_UINT:     return GL_RG16UI;
        case TextureFormat::RG16_SINT:     return GL_RG16I;
        case TextureFormat::RG16_FLOAT:    return GL_RG16F;

        case TextureFormat::RGB16_UNORM:   return GL_RGB16;
        case TextureFormat::RGB16_SNORM:   return GL_RGB16_SNORM;
        case TextureFormat::RGB16_UINT:    return GL_RGB16UI;
        case TextureFormat::RGB16_SINT:    return GL_RGB16I;
        case TextureFormat::RGB16_FLOAT:   return GL_RGB16F;

        case TextureFormat::RGBA16_UNORM:  return GL_RGBA16;
        case TextureFormat::RGBA16_SNORM:  return GL_RGBA16_SNORM;
        case TextureFormat::RGBA16_UINT:   return GL_RGBA16UI;
        case TextureFormat::RGBA16_SINT:   return GL_RGBA16I;
        case TextureFormat::RGBA16_FLOAT:  return GL_RGBA16F;

        // --- 32-bit ---
        case TextureFormat::R32_UINT:      return GL_R32UI;
        case TextureFormat::R32_SINT:      return GL_R32I;
        case TextureFormat::R32_FLOAT:     return GL_R32F;

        case TextureFormat::RG32_UINT:     return GL_RG32UI;
        case TextureFormat::RG32_SINT:     return GL_RG32I;
        case TextureFormat::RG32_FLOAT:    return GL_RG32F;

        case TextureFormat::RGB32_FLOAT:   return GL_RGB32F;
        case TextureFormat::RGBA32_FLOAT:  return GL_RGBA32F;

        // --- Packed ---
        case TextureFormat::RGB10A2_UNORM: return GL_RGB10_A2;
        case TextureFormat::RG11B10_FLOAT: return GL_R11F_G11F_B10F;

        // --- Depth / Stencil ---
        case TextureFormat::S8_UINT:             return GL_STENCIL_INDEX8;
        case TextureFormat::D16_UNORM:           return GL_DEPTH_COMPONENT16;
        case TextureFormat::D24_UNORM_S8_UINT:   return GL_DEPTH24_STENCIL8;
        case TextureFormat::D32_FLOAT:           return GL_DEPTH_COMPONENT32F;
        case TextureFormat::D32_FLOAT_S8_UINT:   return GL_DEPTH32F_STENCIL8;

        // --- BC (S3TC) ---
        case TextureFormat::BC1_UNORM:     return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        case TextureFormat::BC1_SRGB:      return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
        case TextureFormat::BC3_UNORM:     return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        case TextureFormat::BC3_SRGB:      return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
        case TextureFormat::BC4_UNORM:     return GL_COMPRESSED_RED_RGTC1;
        case TextureFormat::BC4_SNORM:     return GL_COMPRESSED_SIGNED_RED_RGTC1;
        case TextureFormat::BC5_UNORM:     return GL_COMPRESSED_RG_RGTC2;
        case TextureFormat::BC5_SNORM:     return GL_COMPRESSED_SIGNED_RG_RGTC2;
        case TextureFormat::BC6H_UFLOAT:   return GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT;
        case TextureFormat::BC6H_SFLOAT:   return GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT;
        case TextureFormat::BC7_UNORM:     return GL_COMPRESSED_RGBA_BPTC_UNORM;
        case TextureFormat::BC7_SRGB:      return GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM;

        // --- ASTC (GLES mobile) ---
        case TextureFormat::ASTC_4x4_UNORM: return GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
        case TextureFormat::ASTC_4x4_SRGB:  return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR;
        case TextureFormat::ASTC_6x6_UNORM: return GL_COMPRESSED_RGBA_ASTC_6x6_KHR;
        case TextureFormat::ASTC_6x6_SRGB:  return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR;
        case TextureFormat::ASTC_8x8_UNORM: return GL_COMPRESSED_RGBA_ASTC_8x8_KHR;
        case TextureFormat::ASTC_8x8_SRGB:  return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR;

        default: return GL_NONE;
    }
}

GLenum ToGLTextureFormat(TextureFormat fmt) {
    switch (fmt) {
        case TextureFormat::R8_UNORM:
        case TextureFormat::R8_SNORM:
        case TextureFormat::R8_UINT:
        case TextureFormat::R8_SINT:
        case TextureFormat::R16_UNORM:
        case TextureFormat::R16_SNORM:
        case TextureFormat::R16_UINT:
        case TextureFormat::R16_SINT:
        case TextureFormat::R16_FLOAT:
        case TextureFormat::R32_UINT:
        case TextureFormat::R32_SINT:
        case TextureFormat::R32_FLOAT:
            return GL_RED;

        case TextureFormat::RG8_UNORM:
        case TextureFormat::RG8_SNORM:
        case TextureFormat::RG8_UINT:
        case TextureFormat::RG8_SINT:
        case TextureFormat::RG16_UNORM:
        case TextureFormat::RG16_SNORM:
        case TextureFormat::RG16_UINT:
        case TextureFormat::RG16_SINT:
        case TextureFormat::RG16_FLOAT:
        case TextureFormat::RG32_UINT:
        case TextureFormat::RG32_SINT:
        case TextureFormat::RG32_FLOAT:
            return GL_RG;

        case TextureFormat::RGB8_UNORM:
        case TextureFormat::RGB8_SNORM:
        case TextureFormat::RGB8_UINT:
        case TextureFormat::RGB8_SINT:
        case TextureFormat::RGB32_FLOAT:
        case TextureFormat::RGB16_UNORM:
        case TextureFormat::RGB16_SNORM:
        case TextureFormat::RGB16_UINT:
        case TextureFormat::RGB16_SINT:
        case TextureFormat::RGB16_FLOAT:
        case TextureFormat::RG11B10_FLOAT:
            return GL_RGB;

        case TextureFormat::RGBA8_UNORM:
        case TextureFormat::RGBA8_SNORM:
        case TextureFormat::RGBA8_UINT:
        case TextureFormat::RGBA8_SINT:
        case TextureFormat::RGBA8_SRGB:
        case TextureFormat::RGBA16_UNORM:
        case TextureFormat::RGBA16_SNORM:
        case TextureFormat::RGBA16_UINT:
        case TextureFormat::RGBA16_SINT:
        case TextureFormat::RGBA16_FLOAT:
        case TextureFormat::RGBA32_FLOAT:
        case TextureFormat::RGB10A2_UNORM:
            return GL_RGBA;

        case TextureFormat::BGRA8_UNORM:
            return GL_BGRA;

        case TextureFormat::S8_UINT:
            return GL_STENCIL_INDEX;
        case TextureFormat::D16_UNORM:
        case TextureFormat::D32_FLOAT:
            return GL_DEPTH_COMPONENT;
        case TextureFormat::D24_UNORM_S8_UINT:
        case TextureFormat::D32_FLOAT_S8_UINT:
            return GL_DEPTH_STENCIL;

        default:
            return GL_NONE;
    }
}

GLenum ToGLTextureType(TextureFormat fmt) {
    switch (fmt) {
        // -------- 8-bit --------
        case TextureFormat::R8_UNORM:
        case TextureFormat::RG8_UNORM:
        case TextureFormat::RGB8_UNORM:
        case TextureFormat::RGBA8_UNORM:
        case TextureFormat::BGRA8_UNORM:
        case TextureFormat::RGBA8_SRGB:
            return GL_UNSIGNED_BYTE;
        case TextureFormat::R8_SNORM:
        case TextureFormat::RG8_SNORM:
        case TextureFormat::RGB8_SNORM:
        case TextureFormat::RGBA8_SNORM:
            return GL_BYTE;
        case TextureFormat::R8_UINT:
        case TextureFormat::RG8_UINT:
        case TextureFormat::RGB8_UINT:
        case TextureFormat::RGBA8_UINT:
            return GL_UNSIGNED_BYTE;
        case TextureFormat::R8_SINT:
        case TextureFormat::RG8_SINT:
        case TextureFormat::RGB8_SINT:
        case TextureFormat::RGBA8_SINT:
            return GL_BYTE;

        // -------- 16-bit --------
        case TextureFormat::R16_UNORM:
        case TextureFormat::RG16_UNORM:
        case TextureFormat::RGBA16_UNORM:
            return GL_UNSIGNED_SHORT;
        case TextureFormat::R16_SNORM:
        case TextureFormat::RG16_SNORM:
        case TextureFormat::RGB16_SNORM:
        case TextureFormat::RGBA16_SNORM:
            return GL_SHORT;
        case TextureFormat::R16_UINT:
        case TextureFormat::RG16_UINT:
        case TextureFormat::RGB16_UINT:
        case TextureFormat::RGBA16_UINT:
            return GL_UNSIGNED_SHORT;
        case TextureFormat::R16_SINT:
        case TextureFormat::RG16_SINT:
        case TextureFormat::RGB16_SINT:
        case TextureFormat::RGBA16_SINT:
            return GL_SHORT;
        case TextureFormat::R16_FLOAT:
        case TextureFormat::RG16_FLOAT:
        case TextureFormat::RGB16_FLOAT:
        case TextureFormat::RGBA16_FLOAT:
            return GL_HALF_FLOAT;

        // -------- 32-bit --------
        case TextureFormat::R32_UINT:
        case TextureFormat::RG32_UINT:
            return GL_UNSIGNED_INT;
        case TextureFormat::R32_SINT:
        case TextureFormat::RG32_SINT:
            return GL_INT;
        case TextureFormat::R32_FLOAT:
        case TextureFormat::RG32_FLOAT:
        case TextureFormat::RGB32_FLOAT:
        case TextureFormat::RGBA32_FLOAT:
            return GL_FLOAT;

        // -------- Packed --------
        case TextureFormat::RGB10A2_UNORM:
            return GL_UNSIGNED_INT_2_10_10_10_REV;
        case TextureFormat::RG11B10_FLOAT:
            return GL_UNSIGNED_INT_10F_11F_11F_REV;

        // -------- Depth / Stencil --------
        case TextureFormat::S8_UINT:
            return GL_UNSIGNED_BYTE;
        case TextureFormat::D16_UNORM:
            return GL_UNSIGNED_SHORT;
        case TextureFormat::D24_UNORM_S8_UINT:
            return GL_UNSIGNED_INT_24_8;
        case TextureFormat::D32_FLOAT:
            return GL_FLOAT;
        case TextureFormat::D32_FLOAT_S8_UINT:
            return GL_FLOAT_32_UNSIGNED_INT_24_8_REV;

        // -------- Compressed --------
        case TextureFormat::BC1_UNORM:
        case TextureFormat::BC1_SRGB:
        case TextureFormat::BC3_UNORM:
        case TextureFormat::BC3_SRGB:
        case TextureFormat::BC4_UNORM:
        case TextureFormat::BC4_SNORM:
        case TextureFormat::BC5_UNORM:
        case TextureFormat::BC5_SNORM:
        case TextureFormat::BC6H_UFLOAT:
        case TextureFormat::BC6H_SFLOAT:
        case TextureFormat::BC7_UNORM:
        case TextureFormat::BC7_SRGB:
        case TextureFormat::ASTC_4x4_UNORM:
        case TextureFormat::ASTC_4x4_SRGB:
        case TextureFormat::ASTC_6x6_UNORM:
        case TextureFormat::ASTC_6x6_SRGB:
        case TextureFormat::ASTC_8x8_UNORM:
        case TextureFormat::ASTC_8x8_SRGB:
            return GL_NONE;
        default:
            return GL_NONE;
    }
}

GLenum ToGLTextureTarget(TextureType type) {
    switch (type) {
        case TextureType::Texture2D:     return GL_TEXTURE_2D;
        case TextureType::Texture3D:     return GL_TEXTURE_3D;
        case TextureType::Cubemap:       return GL_TEXTURE_CUBE_MAP;
        case TextureType::Array2D:       return GL_TEXTURE_2D_ARRAY;
        default: return GL_NONE;
    }
}

GLenum ToGLMinFilter(TextureFilter f, MipmapFilter mf) {
    switch (mf) {
        case MipmapFilter::None:
            return f == TextureFilter::Nearest ? GL_NEAREST : GL_LINEAR;
        case MipmapFilter::Nearest:
            return f == TextureFilter::Nearest ? GL_NEAREST_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_NEAREST;
        case MipmapFilter::Linear:
            return f == TextureFilter::Nearest ? GL_NEAREST_MIPMAP_LINEAR : GL_LINEAR_MIPMAP_LINEAR;
    }
    return GL_LINEAR;
}

GLenum ToGLMagFilter(TextureFilter f) {
    return f == TextureFilter::Nearest ? GL_NEAREST : GL_LINEAR;
}

GLenum ToGLShaderStage(ShaderStage stg) {
    switch (stg) {
        case ShaderStage::Vertex:   return GL_VERTEX_SHADER;
        case ShaderStage::Fragment: return GL_FRAGMENT_SHADER;
        case ShaderStage::Compute:  return GL_COMPUTE_SHADER;
        case ShaderStage::Geometry: return GL_GEOMETRY_SHADER;
        default: return 0;
    }
}

GLenum ToGLVertexAttribType(VertexAttributeType type) {
    switch (type) {
        case VertexAttributeType::Int8:     return GL_BYTE;
        case VertexAttributeType::UInt8:    return GL_UNSIGNED_BYTE;
        case VertexAttributeType::Int16:    return GL_SHORT;
        case VertexAttributeType::UInt16:   return GL_UNSIGNED_SHORT;
        case VertexAttributeType::Int32:    return GL_INT;
        case VertexAttributeType::UInt32:   return GL_UNSIGNED_INT;

        case VertexAttributeType::Float16:  return GL_HALF_FLOAT;
        case VertexAttributeType::Float32:  return GL_FLOAT;
        case VertexAttributeType::Float64:  return GL_DOUBLE;

        default: return GL_INVALID_ENUM;
    }
}

GLenum ToGLCompare(CompareOp cmpop) {
    switch (cmpop) {
        case CompareOp::Never:            return GL_NEVER;
        case CompareOp::Less:             return GL_LESS;
        case CompareOp::Equal:            return GL_EQUAL;
        case CompareOp::LessOrEqual:      return GL_LEQUAL;
        case CompareOp::Greater:          return GL_GREATER;
        case CompareOp::NotEqual:         return GL_NOTEQUAL;
        case CompareOp::GreaterOrEqual:   return GL_GEQUAL;
        case CompareOp::Always:           return GL_ALWAYS;
    }
    return GL_ALWAYS; // fallback
}

GLenum ToGLBlendFactor(BlendFactor bFactor) {
    switch (bFactor) {
        case BlendFactor::Zero:                    return GL_ZERO;
        case BlendFactor::One:                     return GL_ONE;

        case BlendFactor::SrcColor:                return GL_SRC_COLOR;
        case BlendFactor::OneMinusSrcColor:        return GL_ONE_MINUS_SRC_COLOR;

        case BlendFactor::DstColor:                return GL_DST_COLOR;
        case BlendFactor::OneMinusDstColor:        return GL_ONE_MINUS_DST_COLOR;

        case BlendFactor::SrcAlpha:                return GL_SRC_ALPHA;
        case BlendFactor::OneMinusSrcAlpha:        return GL_ONE_MINUS_SRC_ALPHA;

        case BlendFactor::DstAlpha:                return GL_DST_ALPHA;
        case BlendFactor::OneMinusDstAlpha:        return GL_ONE_MINUS_DST_ALPHA;

        case BlendFactor::ConstantColor:           return GL_CONSTANT_COLOR;
        case BlendFactor::OneMinusConstantColor:   return GL_ONE_MINUS_CONSTANT_COLOR;

        case BlendFactor::ConstantAlpha:           return GL_CONSTANT_ALPHA;
        case BlendFactor::OneMinusConstantAlpha:   return GL_ONE_MINUS_CONSTANT_ALPHA;
    }
    return GL_ONE;
}

GLenum ToGLBlendOp(BlendOp bOp) {
    switch (bOp) {
        case BlendOp::Add:              return GL_FUNC_ADD;
        case BlendOp::Subtract:         return GL_FUNC_SUBTRACT;
        case BlendOp::ReverseSubtract:  return GL_FUNC_REVERSE_SUBTRACT;
        case BlendOp::Min:              return GL_MIN;
        case BlendOp::Max:              return GL_MAX;
    }
    return GL_FUNC_ADD;
}

GLenum ToGLStencilOp(StencilOp sOp) {
    switch (sOp) {
        case StencilOp::Keep:              return GL_KEEP;
        case StencilOp::Zero:              return GL_ZERO;
        case StencilOp::Replace:           return GL_REPLACE;
        case StencilOp::IncrementAndClamp: return GL_INCR;
        case StencilOp::DecrementAndClamp: return GL_DECR;
        case StencilOp::Invert:            return GL_INVERT;
        case StencilOp::IncrementAndWrap:  return GL_INCR_WRAP;
        case StencilOp::DecrementAndWrap:  return GL_DECR_WRAP;
    }
    return GL_KEEP;
}

GLenum ToGLBarrierBit(ResourceState state) {
    switch (state) {
        case ResourceState::ComputeRead:
        case ResourceState::ComputeWrite:
            return GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
        case ResourceState::FragmentRead:
            return GL_TEXTURE_FETCH_BARRIER_BIT;
        case ResourceState::VertexRead:
            return GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT;
        case ResourceState::IndirectRead:
            return GL_COMMAND_BARRIER_BIT;
        case ResourceState::RenderTarget:
        case ResourceState::DepthWrite:
            return GL_FRAMEBUFFER_BARRIER_BIT;
        default: return 0;
    }
}

GLenum ToGLPolyMode(PolyMode polyMode) {
    switch (polyMode) {
        case PolyMode::Dots:        return GL_POINTS;
        case PolyMode::Lines:       return GL_LINES;
        case PolyMode::Triangles:   return GL_TRIANGLES;
        default: return GL_NONE;
    }
}

GLenum ToGLBufferTarget(BufferUsage usage) {
    switch (usage) {
        case BufferUsage::Vertex:   return GL_ARRAY_BUFFER;
        case BufferUsage::Index:    return GL_ELEMENT_ARRAY_BUFFER;
        case BufferUsage::Uniform:  return GL_UNIFORM_BUFFER;
        case BufferUsage::Indirect: return GL_DRAW_INDIRECT_BUFFER;
        case BufferUsage::Storage:  return GL_SHADER_STORAGE_BUFFER;    
        default:                    return GL_ARRAY_BUFFER;
    }
}

GLenum ToGLBufferAccess(BufferAccess access) {
    switch (access) {
        case BufferAccess::Dynamic:     return GL_DYNAMIC_DRAW;
        case BufferAccess::Immutable:   return GL_STATIC_DRAW;
        case BufferAccess::Stream:      return GL_STREAM_DRAW;
        default: return GL_NONE;
    }
}

} // namespace axle::gfx
#endif
