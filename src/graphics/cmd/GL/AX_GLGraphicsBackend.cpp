#include "axle/graphics/cmd/GL/AX_GLCommandList.hpp"
#ifdef __AX_GRAPHICS_GL__

#include "axle/graphics/cmd/GL/AX_GLGraphicsBackend.hpp"

#ifdef __AX_PLATFORM_WIN32__
#include "axle/core/ctx/GL/AX_RenderContextGLWin32.hpp"
#elif defined(__AX_PLATFORM_X11__)
#include "axle/core/ctx/GL/AX_RenderContextGLX11.hpp"
#endif

#include "axle/graphics/AX_Graphics.hpp"
#include "axle/utils/AX_Expected.hpp"

#include <slang.h>
#include <slang-com-ptr.h>
#include <slang-com-helper.h>

#include <glad/gl.h>

#include <cstddef>
#include <unordered_set>
#include <string>
#include <cstring>
#include <vector>

using namespace axle::utils;

namespace axle::gfx {

GLContextGuard::GLContextGuard(SharedPtr<core::IRenderContext> ctx) : m_RCtx(ctx) {
#ifdef __AX_PLATFORM_WIN32__
    std::static_pointer_cast<core::RenderContextGLWin32>(m_RCtx)->MakeCurrent();
#elif defined(__AX_PLATFORM_X11__)
    std::static_pointer_cast<core::RenderContextGLX11>(m_RCtx)->MakeCurrent();
#endif
}

GLContextGuard::~GLContextGuard() {
#ifdef __AX_PLATFORM_WIN32__
    std::static_pointer_cast<core::RenderContextGLWin32>(m_RCtx)->ReleaseCurrent();
#elif defined(__AX_PLATFORM_X11__)
    std::static_pointer_cast<core::RenderContextGLX11>(m_RCtx)->ReleaseCurrent();
#endif
}

GLGraphicsBackend::GLGraphicsBackend(SharedPtr<core::IRenderContext> context) : m_Context(context) {
    slang::createGlobalSession(m_SlangGlobal.writeRef());
#ifdef __AX_PLATFORM_WIN32__
    auto handle = std::static_pointer_cast<core::GLCHandleWin32>(m_Context->GetContextHandle());
    m_GL = &handle->glCtx;
#elif defined(__AX_PLATFORM_X11__)
    auto handle = std::static_pointer_cast<core::GLCHandleX11>(m_Context->GetContextHandle());
    m_GL = &handle->glCtx;
#endif
    m_Capabilities = ExpectOrThrow(QueryCaps());

    // emulate swapchain as a single backbuffer for GL
    auto& fb = ReserveHandle(m_Framebuffers, m_FreeFramebuffers);
    fb.fbo = 0;
    fb.hasResolve = false;
    fb.resolveFbo = 0;
    fb.renderPass = {};

    fb.width = handle->surfaceInfo.width;
    fb.height = handle->surfaceInfo.height;

    fb.hasDepth = handle->surfaceInfo.depthBits > 0;
    fb.hasStencil = handle->surfaceInfo.stenctilBits > 0;
    fb.depthStencilTexture = 0; // default FBO owns it
    fb.isSwapchain = true;
    fb.alive = true;

    m_DefaultBackbuffer = FramebufferHandle{fb.index, fb.generation};

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
    GLContextGuard glScope(m_Context);
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
        extensions.insert((const char*) GL_CALL(m_GL->GetStringi(GL_EXTENSIONS, i)));
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

utils::ExResult<SwapchainHandle> GLGraphicsBackend::CreateSwapchain(const SwapchainDesc& desc) {
    return SwapchainHandle{0, 1};
}

utils::AXError GLGraphicsBackend::DestroySwapchain(const SwapchainHandle& desc) {
    // GL implicit
    return AXError::NoError();
}

utils::AXError GLGraphicsBackend::ResizeSwapchain(const SwapchainHandle& handle, uint32_t width, uint32_t height) {
    m_Swapchain.width = width;
    m_Swapchain.height = height;
    return AXError::NoError();
}

ExResult<BufferHandle> GLGraphicsBackend::CreateBuffer(const BufferDesc& desc) {
    GLContextGuard glScope(m_Context);
    if (desc.usage == BufferUsage::Storage && !m_Capabilities.Has(GraphicsCapEnum::ShaderStorageBuffers))
        return AXError{ "Storage buffers not supported on this OpenGL backend (requires 4.3+)" };

    if (desc.usage == BufferUsage::Indirect && !m_Capabilities.Has(GraphicsCapEnum::IndirectDraw))
        return AXError{ "Indirect buffers not supported on this backend" };

    // Staging buffer is fine in GL 3.3 (just normal buffer with CPU updates)
    auto& buff = ReserveHandle(m_Buffers, m_FreeBuffers);

    GLuint id = 0;
    GL_CALL(m_GL->GenBuffers(1, &id));
    GL_CALL(m_GL->BindBuffer(GL_ARRAY_BUFFER, id));

    GLenum usageHint =
        desc.access == BufferAccess::Immutable ? GL_STATIC_DRAW :
        desc.access == BufferAccess::Dynamic   ? GL_DYNAMIC_DRAW :
                                                     GL_STREAM_DRAW;

    GL_CALL(m_GL->BufferData(GL_ARRAY_BUFFER, desc.size, nullptr, usageHint));

    buff.id = id;
    buff.usage = desc.usage;
    buff.size = desc.size;
    buff.slot = desc.bindSlot;
    buff.alive = true;

    return BufferHandle{buff.index, buff.generation};
}

AXError GLGraphicsBackend::UpdateBuffer(const BufferHandle& handle, size_t offset, size_t size, const void* data) {
    GLContextGuard glScope(m_Context);
    if (!IsValidHandle(m_Buffers, handle))
        return { "Invalid Handle" };

    auto& buff = m_Buffers[handle.index];

    if (offset + size > buff.size)
        return { "UpdateBuffer out of bounds" };

    GL_CALL(m_GL->BindBuffer(GL_ARRAY_BUFFER, buff.id));
    GL_CALL(m_GL->BufferSubData(GL_ARRAY_BUFFER, offset, size, data));

    return AXError::NoError();
}

AXError GLGraphicsBackend::DestroyBuffer(const BufferHandle& handle) {
    GLContextGuard glScope(m_Context);
    if (!IsValidHandle(m_Buffers, handle))
        return { "Invalid Handle" };

    auto& buff = m_Buffers[handle.index];

    GL_CALL(m_GL->DeleteBuffers(1, &buff.id));

    buff.id = 0;
    buff.alive = false;

    PostDeleteHandle(buff, handle, m_FreeBuffers);
    return AXError::NoError();
}

ExResult<TextureHandle> GLGraphicsBackend::CreateTexture(const TextureDesc& desc) {
    GLContextGuard glScope(m_Context);
    if (desc.width == 0 || desc.height == 0)
        return AXError{"Invalid dimensions"};

    auto& tex = ReserveHandle(m_Textures, m_FreeTextures);
    
    GLenum target = ToGLTarget(desc.type);
    GLenum internalFormat = ToGLInternalFormat(desc.format);
    GLenum format = ToGLFormat(desc.format);
    GLenum type = ToGLType(desc.format);
    bool compressed = TextureFormatIsS3TC(desc.format) || TextureFormatIsASTC(desc.format);

    GL_CALL(m_GL->GenTextures(1, &tex.id));
    GL_CALL(m_GL->BindTexture(target, tex.id));
    GL_CALL(m_GL->PixelStorei(GL_UNPACK_ALIGNMENT, 1)); // safest: 1, but slightly less optimal for some GPUs

    switch (desc.type) {
        case TextureType::Texture2D:
            if (compressed) {
                GL_CALL(m_GL->CompressedTexImage2D(
                    target,
                    desc.subDesc.mipLevels,
                    format,
                    desc.width,
                    desc.height,
                    0,
                    (GLsizei) desc.pixelsByLayers.size(),
                    desc.pixelsByLayers.data()
                ));
            } else {
                GL_CALL(m_GL->TexImage2D(
                    target,
                    desc.subDesc.mipLevels,
                    internalFormat,
                    desc.width,
                    desc.height,
                    0,
                    format,
                    type,
                    desc.pixelsByLayers.data()
                ));
            }
        break;
        case TextureType::Array2D:
        case TextureType::Texture3D:
            if (compressed) {
                GL_CALL(m_GL->CompressedTexImage3D(
                    target,
                    0,
                    format,
                    desc.width,
                    desc.height,
                    desc.layers,
                    0, 
                    (GLsizei) desc.pixelsByLayers.size(),
                    desc.pixelsByLayers.data()
                ));
            } else {
                GL_CALL(m_GL->TexImage3D(
                    target,
                    0,
                    internalFormat,
                    desc.width,
                    desc.height,
                    desc.layers,
                    0,
                    format,
                    type,
                    desc.pixelsByLayers.data()
                ));
            }
        break;
        case TextureType::Cubemap:
            for (unsigned int i = 0; i < 6; i++) {
                if (compressed) {
                    GL_CALL(m_GL->CompressedTexImage2D(
                        GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                        0,
                        format,
                        desc.width,
                        desc.height,
                        0,
                        (GLsizei) desc.pixelsByCubemap[i].size(),
                        desc.pixelsByCubemap[i].data()
                    ));
                } else {
                    GL_CALL(m_GL->TexImage2D(
                        GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                        0,
                        internalFormat,
                        desc.width,
                        desc.height,
                        0,
                        format,
                        type,
                        desc.pixelsByCubemap[i].data()
                    ));
                }
            }
        break;
    }

    GL_CALL(m_GL->TexParameteri(target, GL_TEXTURE_WRAP_S, ToGLWrap(desc.subDesc.wrapS)));
    GL_CALL(m_GL->TexParameteri(target, GL_TEXTURE_WRAP_T, ToGLWrap(desc.subDesc.wrapT)));
    GL_CALL(m_GL->TexParameteri(target, GL_TEXTURE_WRAP_R, ToGLWrap(desc.subDesc.wrapR)));

    GL_CALL(m_GL->TexParameteri(target, GL_TEXTURE_MIN_FILTER, ToGLFilter(desc.subDesc.minFilter)));
    GL_CALL(m_GL->TexParameteri(target, GL_TEXTURE_MAG_FILTER, ToGLFilter(desc.subDesc.magFilter)));

    if (desc.subDesc.wrapS == TextureWrap::ClampToBorder ||
        desc.subDesc.wrapT == TextureWrap::ClampToBorder ||
        desc.subDesc.wrapR == TextureWrap::ClampToBorder)
        GL_CALL(m_GL->TexParameterfv(target, GL_TEXTURE_BORDER_COLOR, &desc.subDesc.borderColor.r));

    if (desc.subDesc.generateMips && desc.subDesc.mipLevels == 0)
        GL_CALL(m_GL->GenerateMipmap(target));

    if ((desc.subDesc.generateMips || desc.subDesc.mipLevels > 0) && desc.subDesc.aniso > 0.0f) {
        if (!m_Capabilities.Has(GraphicsCapEnum::Anisotropy)) {
            return AXError{"Anisotropic filtering is Unsupported on this device"};
        }
        if (desc.subDesc.aniso > m_Capabilities.maxAniso) {
            return AXError{"Maximum supported anisotropy on this device is " + std::to_string(m_Capabilities.maxAniso)};
        }
        GL_CALL(m_GL->TexParameterf(
            GL_TEXTURE_2D,
            GL_TEXTURE_MAX_ANISOTROPY_EXT,
            desc.subDesc.aniso
        ));
    }

    GL_CALL(m_GL->BindTexture(target, 0));

    tex.alive = true;
    tex.id = tex.id;

    return TextureHandle{tex.index, tex.generation};
}

AXError GLGraphicsBackend::UpdateTexture(const TextureHandle& h, const TextureSubDesc& subDesc, const void* data) {
    GLContextGuard glScope(m_Context);
    if (!IsValidHandle(m_Textures, h))
        return {"Invalid Handle"};

    auto& tex = m_Textures[h.index];
    auto& desc = tex.desc;

    GLenum target = ToGLTarget(desc.type);
    GLenum format = ToGLFormat(desc.format);
    GLenum type = ToGLType(desc.format);
    bool compressed = TextureFormatIsASTC(desc.format) || TextureFormatIsS3TC(desc.format);

    GL_CALL(m_GL->BindTexture(target, tex.id));
    GL_CALL(m_GL->PixelStorei(GL_UNPACK_ALIGNMENT, 1)); // safest

    switch (desc.type) {
        case TextureType::Texture2D:
            if (compressed) {
                GL_CALL(m_GL->CompressedTexSubImage2D(
                    target,
                    subDesc.mipLevels,
                    0, 0,
                    desc.width,
                    desc.height,
                    format,
                    desc.pixelsByLayers.size(),
                    data
                ));
            } else {
                GL_CALL(m_GL->TexSubImage2D(
                    target,
                    subDesc.mipLevels,
                    0, 0,
                    desc.width,
                    desc.height,
                    format,
                    type,
                    data
                ));
            }
        break;
        case TextureType::Array2D:
        case TextureType::Texture3D:
            if (compressed) {
                GL_CALL(m_GL->CompressedTexSubImage3D(
                    target,
                    subDesc.mipLevels,
                    0, 0, 0,
                    desc.width,
                    desc.height,
                    desc.layers,
                    format,
                    desc.pixelsByLayers.size(),
                    data
                ));
            } else {
                GL_CALL(m_GL->TexSubImage3D(
                    target,
                    subDesc.mipLevels,
                    0, 0, 0,
                    desc.width,
                    desc.height,
                    desc.layers,
                    format,
                    type,
                    data
                ));
            }
        break;
        case TextureType::Cubemap:
            for (unsigned int i = 0; i < 6; ++i) {
                if (compressed) {
                    GL_CALL(m_GL->CompressedTexSubImage2D(
                        GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                        subDesc.mipLevels,
                        0, 0,
                        desc.width,
                        desc.height,
                        format,
                        (GLsizei) desc.pixelsByCubemap[i].size(),
                        data
                    ));
                } else {
                    GL_CALL(m_GL->TexSubImage2D(
                        GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                        subDesc.mipLevels,
                        0, 0,
                        desc.width,
                        desc.height,
                        format,
                        type,
                        data
                    ));
                }
            }
        break;
    }

    GL_CALL(m_GL->TexParameteri(target, GL_TEXTURE_WRAP_S, ToGLWrap(subDesc.wrapS)));
    GL_CALL(m_GL->TexParameteri(target, GL_TEXTURE_WRAP_T, ToGLWrap(subDesc.wrapT)));
    GL_CALL(m_GL->TexParameteri(target, GL_TEXTURE_WRAP_R, ToGLWrap(subDesc.wrapR)));

    GL_CALL(m_GL->TexParameteri(target, GL_TEXTURE_MIN_FILTER, ToGLFilter(subDesc.minFilter)));
    GL_CALL(m_GL->TexParameteri(target, GL_TEXTURE_MAG_FILTER, ToGLFilter(subDesc.magFilter)));

    if (subDesc.wrapS == TextureWrap::ClampToBorder ||
        subDesc.wrapT == TextureWrap::ClampToBorder ||
        subDesc.wrapR == TextureWrap::ClampToBorder)
        GL_CALL(m_GL->TexParameterfv(target, GL_TEXTURE_BORDER_COLOR, &subDesc.borderColor.r));

    if (subDesc.generateMips && subDesc.mipLevels == 0)
        GL_CALL(m_GL->GenerateMipmap(target));

    if ((subDesc.generateMips || subDesc.mipLevels > 0) && subDesc.aniso > 0.0f) {
        if (!m_Capabilities.Has(GraphicsCapEnum::Anisotropy)) {
            return AXError{"Anisotropic filtering is Unsupported on this device"};
        }
        if (subDesc.aniso > m_Capabilities.maxAniso) {
            return AXError{"Maximum supported anisotropy on this device is " + std::to_string(m_Capabilities.maxAniso)};
        }
        GL_CALL(m_GL->TexParameterf(
            GL_TEXTURE_2D,
            GL_TEXTURE_MAX_ANISOTROPY_EXT,
            subDesc.aniso
        ));
    }

    tex.desc.subDesc = subDesc;
    GL_CALL(m_GL->BindTexture(target, 0));

    return AXError::NoError();
}

AXError GLGraphicsBackend::DestroyTexture(const TextureHandle& handle) {
    GLContextGuard glScope(m_Context);
    if (IsValidHandle(m_Textures, handle))
        return {"Invalid Handle"};
    auto& tex = m_Textures[handle.index];
    GL_CALL(m_GL->DeleteTextures(1, &tex.id));
    tex.id = 0;
    PostDeleteHandle(tex, handle, m_FreeTextures);
    return AXError::NoError();
}

ExResult<FramebufferHandle> GLGraphicsBackend::CreateFramebuffer(const FramebufferDesc& desc) {
    GLContextGuard glScope(m_Context);
    if (!IsValidHandle(m_RenderPasses, desc.renderPass))
        return AXError{"Invalid RenderPass handle"};

    auto& rp = m_RenderPasses[desc.renderPass.index];
    const RenderPassDesc& rpDesc = rp.desc;

    if (desc.colorAttachments.size != rpDesc.colorAttachments.size)
        return AXError{"Framebuffer color attachment count does not match RenderPass"};

    for (uint32_t i = 0; i < desc.colorAttachments.size; ++i) {
        auto texHandle = desc.colorAttachments.data[i];

        if (!IsValidHandle(m_Textures, texHandle)) {
            return AXError{
                "Invalid Texture Handle at index " +
                std::to_string(i)
            };
        }

        auto& tex = m_Textures[texHandle.index];
        const AttachmentDesc& rpAtt = rpDesc.colorAttachments.data[i];

        if (tex.desc.samples != rpAtt.samples) {
            return AXError{
                "Color attachment sample count mismatch at index " +
                std::to_string(i)
            };
        }

        if (tex.desc.format != rpAtt.format) {
            return AXError{
                "Color attachment format mismatch at index " +
                std::to_string(i)
            };
        }

        GLenum glType = ToGLType(tex.desc.format);

        if (glType == GL_HALF_FLOAT && !m_Capabilities.Has(GraphicsCapEnum::HalfFloatColorBuffer)) {
            return AXError{"Host doesn't support 16 bit float Color buffers (Unwritable)"};
        }
        if (glType == GL_FLOAT && !m_Capabilities.Has(GraphicsCapEnum::FullFloatColorBuffer)) {
            return AXError{"Host doesn't support 32 bit float Color buffers (Unwritable)"};
        }

        if (tex.desc.width != desc.width || tex.desc.height != desc.height) {
            return AXError{
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
        return AXError{"Depth attachment presence mismatch between RenderPass and Framebuffer"};
    }

    if (fbHasStencil != rpHasStencil) {
        return AXError{"Stencil attachment presence mismatch between RenderPass and Framebuffer"};
    }

    const bool hasDepth = fbHasDepth && rpHasDepth;
    const bool hasStencil = fbHasStencil && rpHasStencil;

    if (hasDepth || hasStencil) {
        if (!IsValidHandle(m_Textures, desc.depthStencilTexture))
            return AXError{"Invalid depth/stencil texture handle"};

        auto& depthStencilTex = m_Textures[desc.depthStencilTexture.index];
        const AttachmentDesc& rpDepthStencil = rpDesc.depthStencilAttachment;

        if (depthStencilTex.desc.format != rpDepthStencil.format)
            return AXError{"Depth/Stencil attachment format mismatch"};

        if (depthStencilTex.desc.width != desc.width ||
            depthStencilTex.desc.height != desc.height)
            return AXError{"Depth/Stencil attachment size mismatch"};
    }

    auto& fb = ReserveHandle(m_Framebuffers, m_FreeFramebuffers);

    GL_CALL(m_GL->GenFramebuffers(1, &fb.fbo));
    GL_CALL(m_GL->BindFramebuffer(GL_FRAMEBUFFER, fb.fbo));

    for (uint32_t i = 0; i < desc.colorAttachments.size; ++i) {
        auto& tex = m_Textures[desc.colorAttachments.data[i].index];

        GL_CALL(m_GL->FramebufferTexture2D(
            GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0 + i,
            ToGLTarget(tex.desc.type),
            tex.id,
            0
        ));
    }

    if (hasDepth || hasStencil) {
        auto& depthStencilTex = m_Textures[desc.depthStencilTexture.index];
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
            ToGLTarget(depthStencilTex.desc.type),
            depthStencilTex.id,
            0
        ));
        fb.depthStencilTexture = depthStencilTex.id;
    }

    GLenum status = GL_CALL(m_GL->CheckFramebufferStatus(GL_FRAMEBUFFER));
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        GL_CALL(m_GL->DeleteFramebuffers(1, &fb.fbo));
        GL_CALL(m_GL->BindFramebuffer(GL_FRAMEBUFFER, 0));

        return AXError{"Framebuffer incomplete (GL error)"};
    }

    GL_CALL(m_GL->BindFramebuffer(GL_FRAMEBUFFER, 0));

    bool needsResolve = false;
    bool needsDSResolve = false;
    for (size_t i{0}; i < desc.colorAttachments.size; i++) {
        auto& tex = m_Textures[desc.colorAttachments.data[i].index];
        if (tex.desc.samples > SampleCount::Sample1 && rp.desc.colorAttachments.data[i].hasResolve) {
            needsResolve = true;
            break;
        }
    }
    if (hasDepth || hasStencil) {
        auto& dsTex = m_Textures[desc.depthStencilTexture.index];
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
                ToGLInternalFormat(msaaDesc.format),
                msaaDesc.width,
                msaaDesc.height,
                0,
                ToGLFormat(msaaDesc.format),
                ToGLType(msaaDesc.format),
                nullptr
            ));
            GL_CALL(m_GL->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
            GL_CALL(m_GL->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
            
            GL_CALL(m_GL->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
            GL_CALL(m_GL->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

            return resolveTex;
        };

        for (uint32_t i = 0; i < desc.colorAttachments.size; ++i) {
            auto& tex = m_Textures[desc.colorAttachments.data[i].index];
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
            auto& depthTex = m_Textures[desc.depthStencilTexture.index];
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

        GLenum rsstatus = GL_CALL(m_GL->CheckFramebufferStatus(GL_FRAMEBUFFER));
        if (rsstatus != GL_FRAMEBUFFER_COMPLETE) {
            return AXError{"Resolve FBO incomplete"};
        }
    }

    fb.alive  = true;
    fb.hasDepth = hasDepth;
    fb.hasStencil = hasStencil;
    fb.width  = desc.width;
    fb.height = desc.height;
    fb.renderPass = desc.renderPass;

    return FramebufferHandle{fb.index, fb.generation};
}

AXError GLGraphicsBackend::DestroyFramebuffer(const FramebufferHandle& handle) {
    GLContextGuard glScope(m_Context);

    if (IsEqualHandles(handle, m_DefaultBackbuffer))
        return {"Cannot delete singular default backbuffer FBO, OpenGL is implicit"};
    if (!IsValidHandle(m_Framebuffers, handle))
        return {"Invalid Handle"};

    GLFramebuffer& fb = m_Framebuffers[handle.index];

    GL_CALL(m_GL->DeleteFramebuffers(1, &fb.fbo));
    fb.fbo = 0;
    PostDeleteHandle(fb, handle, m_FreeFramebuffers);

    return AXError::NoError();
}

ExResult<ShaderHandle> GLGraphicsBackend::CreateProgram(const ShaderDesc& desc) {
    GLContextGuard glScope(m_Context);
    bool hasCompute{false};
    bool hasOthers{false};
    for (size_t i{0}; i < desc.entryPoints.size; i++) {
        if (desc.entryPoints.data[i].stage == ShaderStage::Compute) {
            hasCompute = true;
        } else {
            hasOthers = true;
        }
    }
    if (hasCompute && hasOthers) {
        return AXError{"Cannot keep fragment/vertex/etc. along with compute in a pipeline program; Compute is separate."};
    }

    auto& program = ReserveHandle(m_Programs, m_FreePrograms);
    program.id = GL_CALL(m_GL->CreateProgram()); 

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
    for (size_t i{0}; i < eps.size; i++) {
        Slang::ComPtr<slang::IEntryPoint> entry;
        module->findEntryPointByName(eps.data[i].name, entry.writeRef());

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

    for (size_t i{0}; i < desc.entryPoints.size; ++i) {
        Slang::ComPtr<slang::IBlob> code;
        linked->getEntryPointCode(
            (SlangInt)i,
            0,
            code.writeRef(),
            nullptr
        );

        const char* glsl = (const char*) code->getBufferPointer();

        std::string glName = desc.entryPoints.data[i].name;
        GLenum glStage = ToGLStage(desc.entryPoints.data[i].stage);

        GLuint shader = GL_CALL(m_GL->CreateShader(glStage));
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

    return ShaderHandle{program.index, program.generation};
}

AXError GLGraphicsBackend::DestroyProgram(const ShaderHandle& handle) {
    GLContextGuard glScope(m_Context);
    if (!IsValidHandle(m_Programs, handle))
        return {"Invalid Handle"};

    GLProgram& program = m_Programs[handle.index];

    GL_CALL(m_GL->DeleteProgram(program.id));
    program.id = 0;
    PostDeleteHandle(program, handle, m_FreePrograms);
    return AXError::NoError();
}

ExResult<RenderPipelineHandle> GLGraphicsBackend::CreateRenderPipeline(const RenderPipelineDesc& desc) {
    GLContextGuard glScope(m_Context);
    if (!IsValidHandle(m_Programs, desc.shader))
        return AXError("Invalid Pipeline Shader");

    auto& pipeline = ReserveHandle(m_RenderPipelines, m_FreeRenderPipelines);
    
    GL_CALL(m_GL->GenVertexArrays(1, &pipeline.vao));
    GL_CALL(m_GL->BindVertexArray(pipeline.vao));
    
    auto& attrSpan = desc.layout.attributes;
    for (size_t i{0}; i < attrSpan.size; i++) {
        auto& attr = attrSpan.data[i];
        GL_CALL(m_GL->EnableVertexAttribArray(attr.location));

        switch (attr.typeDesc._class) {
            case VertexAttributeClass::Float: {
                GL_CALL(m_GL->VertexAttribPointer(
                    attr.location,
                    attr.componentCount,
                    ToGLType(attr.typeDesc.type),
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
                    ToGLType(attr.typeDesc.type),
                    desc.layout.stride,
                    (void*)(uintptr_t)attr.offset
                ));
                break;
            }
            case VertexAttributeClass::Double: {
                if (!m_Capabilities.Has(GraphicsCapEnum::LongPointers) || m_GL->VertexAttribLPointer == nullptr) {
                    return AXError("Invalid vertex attribute type: Host GPU doesn't support 64 bit pointers");
                }
                GL_CALL(m_GL->VertexAttribLPointer(
                    attr.location,
                    attr.componentCount,
                    ToGLType(attr.typeDesc.type),
                    desc.layout.stride,
                    (void*)(uintptr_t)attr.offset
                ));
                break;
            }
        }
        GL_CALL(m_GL->VertexAttribDivisor(attr.location, attr.divisor));
    }
    GL_CALL(m_GL->BindVertexArray(0));

    pipeline.desc = desc;
    pipeline.program = desc.shader;
    pipeline.vaoLayout = desc.layout;
    pipeline.alive = true;

    return RenderPipelineHandle{pipeline.index, pipeline.generation};
}

AXError GLGraphicsBackend::DestroyRenderPipeline(const RenderPipelineHandle& handle) {
    GLContextGuard glScope(m_Context);
    if (!IsValidHandle(m_RenderPipelines, handle)) 
        return {"Invalid Handle"};
    auto& pipeline = m_RenderPipelines[handle.index];
    GL_CALL(m_GL->DeleteVertexArrays(1, &pipeline.vao));
    pipeline.vao = 0;
    PostDeleteHandle(pipeline, handle, m_FreeRenderPipelines);
    return AXError::NoError();
}

ExResult<ComputePipelineHandle> GLGraphicsBackend::CreateComputePipeline(const ComputePipelineDesc& desc) {
    auto& pipeline = ReserveHandle(m_ComputePipelines, m_FreeComputePipelines);
    pipeline.desc = desc;
    pipeline.alive = true;
    return ComputePipelineHandle{pipeline.index, pipeline.generation};
}

AXError GLGraphicsBackend::DestroyComputePipeline(const ComputePipelineHandle& handle) {
    if (!IsValidHandle(m_ComputePipelines, handle))
        return {"Invalid Handle"};
    PostDeleteHandle(m_ComputePipelines[handle.index], handle, m_FreeComputePipelines);
    return AXError::NoError();
}

ExResult<RenderPassHandle> GLGraphicsBackend::CreateRenderPass(const RenderPassDesc& desc) {
    auto& pass = ReserveHandle(m_RenderPasses, m_FreeRenderPasses);
    pass.desc = desc;
    pass.alive = true;
    return RenderPassHandle{pass.index, pass.generation};
}

AXError GLGraphicsBackend::DestroyRenderPass(const RenderPassHandle& handle) {
    if (!IsValidHandle(m_RenderPasses, handle))
        return {"Invalid Handle"};
    PostDeleteHandle(m_RenderPasses[handle.index], handle, m_FreeRenderPasses);
    return AXError::NoError();
}

AXError GLGraphicsBackend::Dispatch(const ICommandList& cmd, uint32_t x, uint32_t y, uint32_t z) {
    GLContextGuard glScope(m_Context);
    if (x > m_Capabilities.maxWorkGroupCount[0] ||
        y > m_Capabilities.maxWorkGroupCount[1] ||
        z > m_Capabilities.maxWorkGroupCount[2]
    ) return AXError{"Dispatch exceeds max workGroup count"};

    if (!m_CurrentComputePipeline.bound) {
        return AXError{"Bad commands, requested Compute Dispatch, but no Compute Pipeline bound!"};
    }
    Execute(static_cast<const GLCommandList&>(cmd));
    GL_CALL(m_GL->DispatchCompute(x, y, z));
    return AXError::NoError();
}

AXError GLGraphicsBackend::Barrier(const ICommandList&, Span<ResourceTransition> transitions) {
    GLContextGuard glScope(m_Context);
    GLenum bits = 0;

    for (size_t i{0}; i < transitions.size; i++) {
        bits |= ToGLBarrierBit(transitions.data[i].newState);
    }
    if (bits != 0) {
        GL_CALL(m_GL->MemoryBarrier(bits));
    }
    return AXError::NoError();
}

AXError GLGraphicsBackend::Execute(const ICommandList& cmd) {
    GLContextGuard glScope(m_Context);
    auto& glCmd = static_cast<const GLCommandList&>(cmd);

    for (const auto& c : glCmd.Commands()) {
        if (c.type >= GLCommandType::BindVertexBuffer) {
            if (!m_CurrentRenderPass.bound)
                return {"GLCommand::Execute \"GLCommandType::BindVertices/DrawXYZ\" Failed: No RenderPass bound, use BeginRenderPass()"};
            if (!m_CurrentRenderPipeline.bound)
                return {"GLCommand::Execute \"GLCommandType::BindVertices/DrawXYZ\" Failed: No RenderPipline bound, use BindRenderPipeline()"};

            if (!IsValidHandle(m_RenderPasses, m_CurrentRenderPass.handle))
                return {"GLCommand::Execute \"GLCommandType::BindVertices/DrawXYZ\" Failed: Invalid RenderPass bound!"};
            if (!IsValidHandle(m_RenderPipelines, m_CurrentRenderPipeline.handle))
                return {"GLCommand::Execute \"GLCommandType::BindVertices/DrawXYZ\" Failed: Invalid RenderPipline bound!"};
        }

        switch (c.type) {
            case GLCommandType::SetViewport: {
                GL_CALL(m_GL->Viewport((float)c.args[0], (float)c.args[1], (float)c.args[2], (float)c.args[3]));
                GL_CALL(m_GL->DepthRange((float)c.args[4], (float)c.args[5]));
                break;
            };
            case GLCommandType::SetScissor: {
                GL_CALL(m_GL->Scissor((float)c.args[0], (float)c.args[1], (float)c.args[2], (float)c.args[3]));
                break;
            }
            case GLCommandType::BeginRenderPass: {
                uint32_t rpindex = c.args[0], rpgeneration = c.args[1];
                if (!IsValidHandle(m_RenderPasses, rpindex, rpgeneration))
                    return {"GLCommand::Execute \"BeginRenderPass\" Failed: Invalid RenderPass handle"};

                uint32_t fbindex = c.args[2], fbgeneration = c.args[3];
                if (!IsValidHandle(m_RenderPipelines, fbindex, fbgeneration))
                    return {"GLCommand::Execute \"BeginRenderPass\" Failed: Invalid Framebuffer handle"};

                GLRenderPass& pass = m_RenderPasses[rpindex];
                GLFramebuffer& fb  = m_Framebuffers[fbindex];

                GL_CALL(m_GL->BindFramebuffer(GL_FRAMEBUFFER, fb.fbo));

                // Set draw buffers
                std::vector<GLenum> drawBuffers(pass.desc.colorAttachments.size);
                for (size_t i = 0; i < pass.desc.colorAttachments.size; ++i)
                    drawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;

                GL_CALL(m_GL->DrawBuffers((GLsizei)drawBuffers.size(), drawBuffers.data()));

                // Clear attachments based on LoadOp
                for (size_t i = 0; i < pass.desc.colorAttachments.size; ++i) {
                    auto& att = pass.desc.colorAttachments.data[i];

                    if (att.load == LoadOp::Clear) {
                        GL_CALL(m_GL->ClearBufferfv(GL_COLOR, (GLint)i, (const GLfloat*)(&c.args[4])));
                    }
                }
                if (fb.hasDepth) {
                    auto& depthStencil = pass.desc.depthStencilAttachment;

                    if (depthStencil.load == LoadOp::Clear) {
                        GL_CALL(m_GL->ClearBufferfv(GL_DEPTH, 0, (const GLfloat*)(&c.args[7])));
                    }
                }
                if (fb.hasStencil) {
                    auto& depthStencil = pass.desc.depthStencilAttachment;

                    if (depthStencil.load == LoadOp::Clear) {
                        GL_CALL(m_GL->ClearBufferiv(GL_STENCIL, 0, (const GLint*)(&c.args[8])));
                    }
                }

                pass.fbInUse.Bind({fbindex, fbgeneration});
                m_CurrentRenderPass.Bind({rpindex, rpgeneration});
                break;
            }
            case GLCommandType::EndRenderPass: {
                uint32_t rpindex = c.args[0], rpgeneration = c.args[1];
                if (!IsValidHandle(m_RenderPasses, rpindex, rpgeneration))
                    return {"GLCommand::Execute \"GLCommandType::EndRenderPass\" Failed: Invalid RenderPass handle"};

                GLRenderPass& pass = m_RenderPasses[rpindex];
                GLFramebuffer& fb = m_Framebuffers[pass.fbInUse.handle.index];

                if (fb.hasResolve) {
                    GL_CALL(m_GL->BindFramebuffer(GL_READ_FRAMEBUFFER, fb.fbo));
                    GL_CALL(m_GL->BindFramebuffer(GL_DRAW_FRAMEBUFFER, fb.resolveFbo));

                    for (size_t i = 0; i < pass.desc.colorAttachments.size; ++i) {
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

                std::vector<GLenum> invalidateAttachments;

                for (size_t i = 0; i < pass.desc.colorAttachments.size; ++i) {
                    if (pass.desc.colorAttachments.data[i].store == StoreOp::Discard)
                        invalidateAttachments.push_back(GL_COLOR_ATTACHMENT0 + i);
                }

                if (pass.desc.depthStencilAttachment.store == StoreOp::Discard)
                    invalidateAttachments.push_back(GL_DEPTH_ATTACHMENT);

                if (!invalidateAttachments.empty()) {
                    GL_CALL(m_GL->InvalidateFramebuffer(
                        GL_FRAMEBUFFER,
                        (GLsizei)invalidateAttachments.size(),
                        invalidateAttachments.data()
                    ));
                }

                GL_CALL(m_GL->BindFramebuffer(GL_FRAMEBUFFER, 0));

                pass.fbInUse.UnBind();
                m_CurrentRenderPass.UnBind();
                break;
            }
            case GLCommandType::BindRenderPipeline: {
                uint32_t index = c.args[0], generation = c.args[1];

                if (!IsValidHandle(m_RenderPipelines, index, generation))
                    return {"GLCommand::Execute \"GLCommandType::BindRenderPipeline\" Failed: Invalid pipeline handle"};

                if (!m_CurrentRenderPass.bound)
                    return {"GLCommand::Execute \"GLCommandType::BindRenderPipeline\" Failed: No RenderPass bound, use BeginRenderPass()"};
                if (!IsValidHandle(m_RenderPasses, m_CurrentRenderPass.handle))
                    return {"GLCommand::Execute \"GLCommandType::BindRenderPipeline\" Failed: Invalid RenderPass bound!"};

                auto& pipeline = m_RenderPipelines[index];

                if (!IsValidHandle(m_Programs, pipeline.program))
                    return {"GLCommand::Execute \"GLCommandType::BindRenderPipeline\" Failed: Invalid Pipeline shader program!"};

                auto& desc = pipeline.desc;

                if (
                    m_CurrentRenderPipeline.bound &&
                    m_CurrentRenderPipeline.handle.index == index &&
                    m_CurrentRenderPipeline.handle.generation == generation
                ) {
                    break;
                }

                m_CurrentRenderPipeline.Bind({index, generation});

                auto& program = m_Programs[pipeline.program.index];
                if (m_CurrentState.program != program.id) {
                    GL_CALL(m_GL->UseProgram(program.id));
                    m_CurrentState.program = program.id;
                }

                bool wantCull = desc.raster.cull != CullMode::None;

                if (m_CurrentState.cullEnabled != wantCull) {
                    m_CurrentState.cullEnabled = wantCull;
                    wantCull ? GL_CALL(m_GL->Enable(GL_CULL_FACE))
                            : GL_CALL(m_GL->Disable(GL_CULL_FACE));
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
                    desc.depth.depthTest ? GL_CALL(m_GL->Enable(GL_DEPTH_TEST))
                                        : GL_CALL(m_GL->Disable(GL_DEPTH_TEST));
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
                    desc.depth.stencilTest ? GL_CALL(m_GL->Enable(GL_STENCIL_TEST))
                                        : GL_CALL(m_GL->Disable(GL_STENCIL_TEST));
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
                    desc.blend.enabled ? GL_CALL(m_GL->Enable(GL_BLEND))
                                    : GL_CALL(m_GL->Disable(GL_BLEND));
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

                if (m_CurrentState.vao != pipeline.vao) {
                    GL_CALL(m_GL->BindVertexArray(pipeline.vao));
                    m_CurrentState.vao = pipeline.vao;
                }
                break;
            }
            case GLCommandType::BindComputePipeline: {
                uint32_t index = c.args[0], generation = c.args[1];

                if (!IsValidHandle(m_ComputePipelines, index, generation))
                    return {"GLCommand::Execute \"GLCommandType::BindComputePipeline\" Failed: Invalid compute-pipeline handle"};
                m_CurrentComputePipeline.Bind({index, generation});
                break;
            }
            case GLCommandType::BindVertexBuffer:
            case GLCommandType::BindIndexBuffer: {
                uint32_t index = c.args[0], generation = c.args[1];
                if (!IsValidHandle(m_Buffers, index, generation))
                    return {"GLCommand::Execute \"GLCommandType::Bind(Vertex/Index)Buffer\" Failed: Invalid buffer handle"};
                auto& buff = m_Buffers[index];
                if (!m_CurrentRenderPipeline.bound)
                    return {"GLCommand::Execute \"GLCommandType::Bind(Vertex/Index)Buffer\" Failed: No RenderPipline bound, use BindRenderPipeline()"};
                if (!IsValidHandle(m_RenderPipelines, m_CurrentRenderPipeline.handle))
                    return {"GLCommand::Execute \"GLCommandType::Bind(Vertex/Index)Buffer\" Failed: Invalid RenderPipeline bound!"};
                
                auto& pipeline = m_RenderPipelines[m_CurrentRenderPipeline.handle.index];
                switch (buff.usage) {
                    case BufferUsage::Vertex: {
                        if (pipeline.vbo != buff.id) {
                            pipeline.vbo = buff.id;
                            GL_CALL(m_GL->BindBuffer(GL_ARRAY_BUFFER, buff.id));
                        }
                    }
                    break;
                    case BufferUsage::Index: {
                        if (pipeline.ebo != buff.id) {
                            pipeline.ebo = buff.id;
                            GL_CALL(m_GL->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, buff.id));
                        }
                    }
                    break;
                    default: return {"GLCommand::Execute \"GLCommandType::Bind(Vertex/Index)Buffer\" \
                        Failed: Illegal buffer handle; must be either Vertex/Index Buffer"};
                }
                break;
            }
            case GLCommandType::BindIndirectBuffer: {
                if (!m_Capabilities.Has(GraphicsCapEnum::IndirectDraw))
                    return {"GLCommand::Execute \"GLCommandType::BindIndirectBuffer\" Failed: Indirect Rendering is Unsupported on this device"};

                uint32_t index = c.args[0], generation = c.args[1];
                if (!IsValidHandle(m_Buffers, index, generation))
                    return {"GLCommand::Execute \"GLCommandType::BindIndirectBuffer\" Failed: Invalid buffer handle"};

                auto& buff = m_Buffers[index];
                if (buff.usage != BufferUsage::Indirect)
                    return {"GLCommand::Execute \"GLCommandType::BindIndirectBuffer\" Failed: Illegal buffer handle: BufferUsage != Indirect"};

                GL_CALL(m_GL->BindBuffer(GL_DRAW_INDIRECT_BUFFER, buff.id));
                break;
            }
            case GLCommandType::BindResourceSet: {
                uint32_t index = c.args[0], generation = c.args[1];
                if (!IsValidHandle(m_ResourceSets, index, generation))
                    return {"GLCommand::Execute \"GLCommandType::BindResourceSet\" Failed: Invalid ResourceSet handle"};
                auto& res = m_ResourceSets[index];

                for (auto& b : res.bindings) {
                    switch (b.type) {
                        case BindingType::UniformBuffer: {
                            if (!IsValidHandle(m_Buffers, b.resource.index, b.resource.generation))
                                return {"GLCommand::Execute \"GLCommandType::BindResourceSet::UniformBuffer\" Failed: Invalid Buffer handle"};

                            GL_CALL(m_GL->BindBufferRange(
                                GL_UNIFORM_BUFFER,
                                b.slot,
                                m_Buffers[b.resource.index].id,
                                b.offset,
                                b.range
                            ));
                            break;
                        }
                        case BindingType::StorageBuffer: {
                            if (!IsValidHandle(m_Buffers, b.resource.index, b.resource.generation))
                                return {"GLCommand::Execute \"GLCommandType::BindResourceSet::StorageBuffer\" Failed: Invalid Buffer handle"};

                            GL_CALL(m_GL->BindBufferRange(
                                GL_SHADER_STORAGE_BUFFER,
                                b.slot,
                                m_Buffers[b.resource.index].id,
                                b.offset,
                                b.range
                            ));
                            break;
                        }
                        case BindingType::SampledTexture: {
                            if (!IsValidHandle(m_Textures, b.resource.index, b.resource.generation))
                                return {"GLCommand::Execute \"GLCommandType::BindResourceSet::SampledTexture\" Failed: Invalid Texture handle"};
                            
                            GL_CALL(m_GL->ActiveTexture(GL_TEXTURE0 + b.slot));
                            GL_CALL(m_GL->BindTexture(GL_TEXTURE_2D, m_Textures[b.resource.index].id));
                            break;
                        }
                        case BindingType::StorageTexture: {
                            if (!IsValidHandle(m_Textures, b.resource.index, b.resource.generation))
                                return {"GLCommand::Execute \"GLCommandType::BindResourceSet::StorageTexture\" Failed: Invalid Texture handle"};

                            GL_CALL(m_GL->BindImageTexture(
                                b.slot,
                                m_Textures[b.resource.index].id,
                                0,
                                GL_FALSE,
                                0,
                                GL_READ_WRITE,
                                ToGLFormat(m_Textures[b.resource.index].desc.format)
                            ));
                            break;
                        }
                    }
                }
                break;
            }
            case GLCommandType::Draw: {
                auto& pipeline = m_RenderPipelines[m_CurrentRenderPipeline.handle.index];
                GL_CALL(m_GL->DrawArrays(ToGLPolyMode(pipeline.desc.raster.polyMode), c.args[1], c.args[0]));
                break;
            }
            case GLCommandType::DrawIndexed: {
                auto& pipeline = m_RenderPipelines[m_CurrentRenderPipeline.handle.index];
                GL_CALL(m_GL->DrawElements(ToGLPolyMode(pipeline.desc.raster.polyMode), c.args[0], GL_UNSIGNED_INT, (void*)c.args[1]));
                break;
            }
            case GLCommandType::DrawInstanced: {
                auto& pipeline = m_RenderPipelines[m_CurrentRenderPipeline.handle.index];
                GL_CALL(m_GL->DrawArraysInstanced(ToGLPolyMode(pipeline.desc.raster.polyMode), c.args[2], c.args[0], c.args[1]));
                break;
            }
            case GLCommandType::DrawIndexedInstanced: {
                auto& pipeline = m_RenderPipelines[m_CurrentRenderPipeline.handle.index];
                GL_CALL(m_GL->DrawElementsInstanced(ToGLPolyMode(pipeline.desc.raster.polyMode), c.args[0], GL_UNSIGNED_INT, (void*)c.args[2], c.args[1]));
                break;
            }
            case GLCommandType::DrawIndirect: {
                if (!m_Capabilities.Has(GraphicsCapEnum::IndirectDraw))
                    return {"GLCommand::Execute \"GLCommandType::DrawIndirect\" Failed: Indirect Rendering is Unsupported on this device"};
                auto& pipeline = m_RenderPipelines[m_CurrentRenderPipeline.handle.index];
                if (c.args[1] > 1) {
                    if (!m_Capabilities.Has(GraphicsCapEnum::MultiDrawIndirect))
                        return {"GLCommand::Execute \"GLCommandType::DrawIndirect\" Failed: Multi-Indirect Rendering is Unsupported on this device"};
                    GL_CALL(m_GL->MultiDrawArraysIndirect(ToGLPolyMode(pipeline.desc.raster.polyMode), (void*)c.args[0], c.args[1], c.args[2]));
                } else {
                    GL_CALL(m_GL->DrawArraysIndirect(ToGLPolyMode(pipeline.desc.raster.polyMode), (void*)c.args[0]));
                }
                break;
            }
            case GLCommandType::DrawIndirectIndexed: {
                if (!m_Capabilities.Has(GraphicsCapEnum::IndirectDraw))
                    return {"GLCommand::Execute \"GLCommandType::DrawIndirectIndexed\" Failed: Indirect Rendering is Unsupported on this device"};
                auto& pipeline = m_RenderPipelines[m_CurrentRenderPipeline.handle.index];
                if (c.args[1] > 1) {
                    if (!m_Capabilities.Has(GraphicsCapEnum::MultiDrawIndirect))
                        return {"GLCommand::Execute \"GLCommandType::DrawIndirectIndexed\" Failed: Multi-Indirect Rendering is Unsupported on this device"};
                    GL_CALL(m_GL->MultiDrawElementsIndirect(ToGLPolyMode(pipeline.desc.raster.polyMode), GL_UNSIGNED_INT, (void*)c.args[0], c.args[1], c.args[2]));
                } else {
                    GL_CALL(m_GL->DrawElementsIndirect(ToGLPolyMode(pipeline.desc.raster.polyMode), GL_UNSIGNED_INT, (void*)c.args[0]));
                }
                break;
            }
        }
    }
    return AXError::NoError();
}

utils::ExResult<uint32_t> GLGraphicsBackend::AcquireNextImage() {
    return 0; // OpenGL always has single backbuffer
}

utils::ExResult<FramebufferHandle> GLGraphicsBackend::GetSwapchainFramebuffer(uint32_t imageIndex) {
    if (imageIndex != 0)
        return AXError{"OpenGL has only one single backbuffer and is (FBO 0)"};
    return m_DefaultBackbuffer;
}

utils::AXError GLGraphicsBackend::Present(uint32_t imageIndex) {
    if (imageIndex != 0)
        return AXError{"OpenGL has only one backbuffer"};

    m_Context->SwapBuffers();
    return AXError::NoError();
}

utils::ExResult<ResourceSetHandle> GLGraphicsBackend::CreateResourceSet(const ResourceSetDesc& desc) {
    auto& handle = ReserveHandle(m_ResourceSets, m_FreeResourceSets);

    handle.bindings.resize(desc.bindings.size);
    for (size_t i{0}; i < desc.bindings.size; i++) {
        handle.bindings[i] = desc.bindings.data[i];
    }
    handle.layoutID = desc.layoutID;
    handle.version = 1;
    handle.alive = true;

    return ResourceSetHandle{handle.index, handle.generation};
}

utils::AXError GLGraphicsBackend::UpdateResourceSet(const ResourceSetHandle& handle, Span<Binding> bindings) {
    if (!IsValidHandle(m_ResourceSets, handle))
        return {"Invalid handle"};
    auto& res = m_ResourceSets[handle.index];

    res.bindings.resize(bindings.size);
    for (size_t i{0}; i < bindings.size; i++) {
        res.bindings[i] = bindings.data[i];
    }
    res.version++;
    return AXError::NoError();
}

utils::AXError GLGraphicsBackend::DestroyResourceSet(const ResourceSetHandle& handle) {
    if (!IsValidHandle(m_ResourceSets, handle))
        return {"Invalid handle"};
    auto& res = m_ResourceSets[handle.index];

    PostDeleteHandle(res, handle, m_FreeResourceSets);
    return AXError::NoError();
}

GLenum ToGLWrap(TextureWrap wrap) {
    switch (wrap) {
        case TextureWrap::Repeat:         return GL_REPEAT;
        case TextureWrap::MirrorRepeat:   return GL_MIRRORED_REPEAT;
        case TextureWrap::ClampToEdge:    return GL_CLAMP_TO_EDGE;
        case TextureWrap::ClampToBorder:  return GL_CLAMP_TO_BORDER;
        default: return GL_NONE;
    }
}

GLenum ToGLFilter(TextureFilter filter, bool mipmapEnabled) {
    switch (filter) {
        case TextureFilter::Nearest:
            return mipmapEnabled ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST;
        case TextureFilter::Linear:
            return mipmapEnabled ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
        default: return GL_NONE;
    }
}

GLenum ToGLInternalFormat(TextureFormat fmt) {
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

GLenum ToGLFormat(TextureFormat fmt) {
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

        case TextureFormat::RGB32_FLOAT:
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

GLenum ToGLType(TextureFormat fmt) {
    switch (fmt) {
        // -------- 8-bit --------
        case TextureFormat::R8_UNORM:
        case TextureFormat::RG8_UNORM:
        case TextureFormat::RGBA8_UNORM:
        case TextureFormat::BGRA8_UNORM:
        case TextureFormat::RGBA8_SRGB:
            return GL_UNSIGNED_BYTE;
        case TextureFormat::R8_SNORM:
        case TextureFormat::RG8_SNORM:
        case TextureFormat::RGBA8_SNORM:
            return GL_BYTE;
        case TextureFormat::R8_UINT:
        case TextureFormat::RG8_UINT:
        case TextureFormat::RGBA8_UINT:
            return GL_UNSIGNED_BYTE;
        case TextureFormat::R8_SINT:
        case TextureFormat::RG8_SINT:
        case TextureFormat::RGBA8_SINT:
            return GL_BYTE;

        // -------- 16-bit --------
        case TextureFormat::R16_UNORM:
        case TextureFormat::RG16_UNORM:
        case TextureFormat::RGBA16_UNORM:
            return GL_UNSIGNED_SHORT;
        case TextureFormat::R16_SNORM:
        case TextureFormat::RG16_SNORM:
        case TextureFormat::RGBA16_SNORM:
            return GL_SHORT;
        case TextureFormat::R16_UINT:
        case TextureFormat::RG16_UINT:
        case TextureFormat::RGBA16_UINT:
            return GL_UNSIGNED_SHORT;
        case TextureFormat::R16_SINT:
        case TextureFormat::RG16_SINT:
        case TextureFormat::RGBA16_SINT:
            return GL_SHORT;
        case TextureFormat::R16_FLOAT:
        case TextureFormat::RG16_FLOAT:
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

GLenum ToGLTarget(TextureType type) {
    switch (type) {
        case TextureType::Texture2D:     return GL_TEXTURE_2D;
        case TextureType::Texture3D:     return GL_TEXTURE_3D;
        case TextureType::Cubemap:       return GL_TEXTURE_CUBE_MAP;
        case TextureType::Array2D:       return GL_TEXTURE_2D_ARRAY;
        default: return GL_NONE;
    }
}

GLenum ToGLStage(ShaderStage stg) {
    switch (stg) {
        case ShaderStage::Vertex:   return GL_VERTEX_SHADER;
        case ShaderStage::Fragment: return GL_FRAGMENT_SHADER;
        case ShaderStage::Compute:  return GL_COMPUTE_SHADER;
        case ShaderStage::Geometry: return GL_GEOMETRY_SHADER;
        default: return 0;
    }
}

GLenum ToGLType(VertexAttributeType type) {
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

GLenum ToGLFilter(TextureFilter filter) {
    switch (filter) {
        case TextureFilter::Nearest:    return GL_NEAREST;
        case TextureFilter::Linear:     return GL_LINEAR;
        default: return GL_NONE;
    }
}

} // namespace axle::gfx
#endif
