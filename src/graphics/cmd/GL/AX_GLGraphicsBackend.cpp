#ifdef __AX_GRAPHICS_GL__

#include "axle/graphics/cmd/GL/AX_GLGraphicsBackend.hpp"

#include "axle/graphics/AX_Graphics.hpp"
#include "axle/utils/AX_Expected.hpp"

#include <slang.h>
#include <slang-com-ptr.h>
#include <slang-com-helper.h>

#include <glad/glad.h>

#include <unordered_set>
#include <string>
#include <cstring>
#include <vector>

using namespace axle::utils;

/* TODO:
    - Create/Bind UBOs & SSBOs and abstract them in a way that mimics Vulkan behvaior
    - Move BindResources() to GLCommand action enum(s)
    - Learn More about Packed Textures
    - Add Multisample Resolve in BindPipeline? or maybe POST-draw? idk
    - Compute Pipeline (Including the ShaderProgram load part, which should have multiple compute entries)
*/

namespace axle::gfx {

GLGraphicsBackend::GLGraphicsBackend() {
    slang::createGlobalSession(m_SlangGlobal.writeRef());
    m_Capabilities = ExpectOrThrow(QueryCaps());
}

GLGraphicsBackend::~GLGraphicsBackend() {
    slang::shutdown();
}

bool GLGraphicsBackend::SupportsCap(GraphicsCapEnum cap) {
    return m_Capabilities.caps[static_cast<int32_t>(cap)];
};

ExResult<GraphicsCaps> GLGraphicsBackend::QueryCaps() {
    GraphicsCaps result{};

    int major = 0, minor = 0;
    GL_CALL(glGetIntegerv(GL_MAJOR_VERSION, &major));
    GL_CALL(glGetIntegerv(GL_MINOR_VERSION, &minor));

    std::unordered_set<std::string> extensions;

    GLint extn = 0;
    GL_CALL(glGetIntegerv(GL_NUM_EXTENSIONS, &extn));
    for (GLint i = 0; i < extn; i++) {
        extensions.insert((const char*) GL_CALL(glGetStringi(GL_EXTENSIONS, i)));
    }
    auto HasExt = [&](const char* ext) {
        return extensions.contains(ext);
    };

    result.caps[(int)GraphicsCapEnum::ComputeShaders] =
        (major > 4 || (major == 4 && minor >= 3)) ||
        HasExt("GL_ARB_compute_shader");

    result.caps[(int)GraphicsCapEnum::ShaderStorageBuffers] =
        (major > 4 || (major == 4 && minor >= 3)) ||
        HasExt("GL_ARB_shader_storage_buffer_object");

    result.caps[(int)GraphicsCapEnum::IndirectDraw] =
        (major >= 4) || HasExt("GL_ARB_draw_indirect");

    result.caps[(int)GraphicsCapEnum::GeometryShaders] =
        (major > 3 || (major == 3 && minor >= 2)) ||
        HasExt("GL_ARB_geometry_shader4");

    result.caps[(int)GraphicsCapEnum::Tessellation] =
        (major > 4 || (major == 4 && minor >= 0)) ||
        HasExt("GL_ARB_tessellation_shader");
        
    result.caps[(int)GraphicsCapEnum::MultiDrawIndirect] =
        (major > 4 || (major == 4 && minor >= 3)) ||
        HasExt("GL_ARB_multi_draw_indirect");
        
    result.caps[(int)GraphicsCapEnum::TextureArray] =
        (major >= 3) ||
        HasExt("GL_EXT_texture_array");

    result.caps[(int)GraphicsCapEnum::SparseTextures] =
        (major > 4 || (major == 4 && minor >= 4)) ||
        HasExt("GL_ARB_sparse_texture");
    
    result.caps[(int)GraphicsCapEnum::RayTracing] =
        HasExt("GL_NV_ray_tracing"); // NVIDIA-Only

    result.caps[(int)GraphicsCapEnum::BindlessTextures] =
        HasExt("GL_ARB_bindless_texture") ||
        HasExt("GL_NV_bindless_texture");

    GL_CALL(glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &result.maxVertexAttribs));

    GL_CALL(glGetIntegerv(GL_MAX_DRAW_BUFFERS, &result.maxDrawBuffers));
    GL_CALL(glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &result.maxColorAttachments));

    GL_CALL(glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &result.maxUBOBindings));
    GL_CALL(glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &result.maxUBOSize));

    GL_CALL(glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &result.maxArrayTextureLayers));
    GL_CALL(glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &result.maxTextureUnits));
    GL_CALL(glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &result.maxCombinedTextureUnits));

    if (result.Has(GraphicsCapEnum::ShaderStorageBuffers)) {
        GL_CALL(glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &result.maxSSBOSize));
        GL_CALL(glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &result.maxSSBOBindings));
    }
    if (result.Has(GraphicsCapEnum::ComputeShaders)) {
        for (int i = 0; i < 3; ++i) {
            GL_CALL(glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, i, (int*)&result.maxWorkGroupCount[i]));
        }
    }

    return result;
}

ExResult<BufferHandle> GLGraphicsBackend::CreateBuffer(const BufferDesc& desc) {
    if (desc.usage == BufferUsage::Storage && !m_Capabilities.Has(GraphicsCapEnum::ShaderStorageBuffers))
        return AXError{ "Storage buffers not supported on this OpenGL backend (requires 4.3+)" };

    if (desc.usage == BufferUsage::Indirect && !m_Capabilities.Has(GraphicsCapEnum::IndirectDraw))
        return AXError{ "Indirect buffers not supported on this backend" };

    // Staging buffer is fine in GL 3.3 (just normal buffer with CPU updates)
    auto& buff = ReserveHandle(m_Buffers, m_FreeBuffers);

    GLuint id = 0;
    GL_CALL(glGenBuffers(1, &id));

    // In GL 3.3, target at allocation time does NOT define buffer type.
    // We use GL_ARRAY_BUFFER as neutral allocation target.
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, id));

    GLenum usageHint =
        desc.access == BufferAccess::Immutable ? GL_STATIC_DRAW :
        desc.access == BufferAccess::Dynamic   ? GL_DYNAMIC_DRAW :
                                                     GL_STREAM_DRAW;

    GL_CALL(glBufferData(GL_ARRAY_BUFFER, desc.size, nullptr, usageHint));

    buff.id = id;
    buff.usage = desc.usage;
    buff.size = desc.size;
    buff.slot = desc.bindSlot;
    buff.alive = true;

    return BufferHandle{buff.index, buff.generation};
}

AXError GLGraphicsBackend::UpdateBuffer(BufferHandle& handle, size_t offset, size_t size, const void* data) {
    if (!IsValidHandle(m_Buffers, handle))
        return { "Invalid Handle" };

    auto& buff = m_Buffers[handle.index];

    if (offset + size > buff.size)
        return { "UpdateBuffer out of bounds" };

    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, buff.id));
    GL_CALL(glBufferSubData(GL_ARRAY_BUFFER, offset, size, data));

    return AXError::NoError();
}

AXError GLGraphicsBackend::DestroyBuffer(BufferHandle& handle) {
    if (!IsValidHandle(m_Buffers, handle))
        return { "Invalid Handle" };

    auto& buff = m_Buffers[handle.index];

    GL_CALL(glDeleteBuffers(1, &buff.id));

    buff.id = 0;
    buff.alive = false;

    PostDeleteHandle(buff, handle, m_FreeBuffers);
    return AXError::NoError();
}

ExResult<TextureHandle> GLGraphicsBackend::CreateTexture(const TextureDesc& desc) {
    if (desc.width == 0 || desc.height == 0)
        return AXError{"Invalid dimensions"};
    if (desc.samples == 0)
        return AXError{"Samples must be >= 1"};

    auto& tex = ReserveHandle(m_Textures, m_FreeTextures);
    
    GLenum target = ToGLTarget(desc.type);
    GLenum internalFormat = ToGLInternalFormat(desc.format);
    GLenum format = ToGLFormat(desc.format);
    GLenum type = ToGLType(desc.format);
    bool compressed = TextureFormatIsS3TC(desc.format) || TextureFormatIsASTC(desc.format);

    GL_CALL(glGenTextures(1, &tex.id));
    GL_CALL(glBindTexture(target, tex.id));
    GL_CALL(glPixelStorei(GL_UNPACK_ALIGNMENT, 1)); // safest: 1, but slightly less optimal for some GPUs

    switch (desc.type) {
        case TextureType::Texture2D:
            if (compressed) {
                GL_CALL(glCompressedTexImage2D(
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
                GL_CALL(glTexImage2D(
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
                GL_CALL(glCompressedTexImage3D(
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
                GL_CALL(glTexImage3D(
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
                    GL_CALL(glCompressedTexImage2D(
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
                    GL_CALL(glTexImage2D(
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

    GL_CALL(glTexParameteri(target, GL_TEXTURE_WRAP_S, ToGLWrap(desc.subDesc.wrapS)));
    GL_CALL(glTexParameteri(target, GL_TEXTURE_WRAP_T, ToGLWrap(desc.subDesc.wrapT)));
    GL_CALL(glTexParameteri(target, GL_TEXTURE_WRAP_R, ToGLWrap(desc.subDesc.wrapR)));

    GL_CALL(glTexParameteri(target, GL_TEXTURE_MIN_FILTER, ToGLFilter(desc.subDesc.minFilter)));
    GL_CALL(glTexParameteri(target, GL_TEXTURE_MAG_FILTER, ToGLFilter(desc.subDesc.magFilter)));

    if (desc.subDesc.wrapS == TextureWrap::ClampToBorder ||
        desc.subDesc.wrapT == TextureWrap::ClampToBorder ||
        desc.subDesc.wrapR == TextureWrap::ClampToBorder)
        GL_CALL(glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, &desc.subDesc.borderColor.r));

    if (desc.subDesc.generateMips && desc.subDesc.mipLevels == 0)
        GL_CALL(glGenerateMipmap(target));

    GL_CALL(glBindTexture(target, 0));

    tex.alive = true;
    tex.id = tex.id;

    return TextureHandle{tex.index, tex.generation};
}

AXError GLGraphicsBackend::UpdateTexture(TextureHandle& h, const TextureSubDesc& subDesc, const void* data) {
    if (!IsValidHandle(m_Textures, h))
        return {"Invalid Handle"};

    auto& tex = m_Textures[h.index];
    auto& desc = tex.desc;

    GLenum target = ToGLTarget(desc.type);
    GLenum format = ToGLFormat(desc.format);
    GLenum type = ToGLType(desc.format);
    bool compressed = TextureFormatIsASTC(desc.format) || TextureFormatIsS3TC(desc.format);

    GL_CALL(glBindTexture(target, tex.id));
    GL_CALL(glPixelStorei(GL_UNPACK_ALIGNMENT, 1)); // safest

    switch (desc.type) {
        case TextureType::Texture2D:
            if (compressed) {
                GL_CALL(glCompressedTexSubImage2D(
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
                GL_CALL(glTexSubImage2D(
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
                GL_CALL(glCompressedTexSubImage3D(
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
                GL_CALL(glTexSubImage3D(
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
                    GL_CALL(glCompressedTexSubImage2D(
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
                    GL_CALL(glTexSubImage2D(
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

    GL_CALL(glTexParameteri(target, GL_TEXTURE_WRAP_S, ToGLWrap(subDesc.wrapS)));
    GL_CALL(glTexParameteri(target, GL_TEXTURE_WRAP_T, ToGLWrap(subDesc.wrapT)));
    GL_CALL(glTexParameteri(target, GL_TEXTURE_WRAP_R, ToGLWrap(subDesc.wrapR)));

    GL_CALL(glTexParameteri(target, GL_TEXTURE_MIN_FILTER, ToGLFilter(subDesc.minFilter)));
    GL_CALL(glTexParameteri(target, GL_TEXTURE_MAG_FILTER, ToGLFilter(subDesc.magFilter)));

    if (subDesc.wrapS == TextureWrap::ClampToBorder ||
        subDesc.wrapT == TextureWrap::ClampToBorder ||
        subDesc.wrapR == TextureWrap::ClampToBorder)
        GL_CALL(glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, &subDesc.borderColor.r));

    if (subDesc.generateMips && subDesc.mipLevels == 0)
        GL_CALL(glGenerateMipmap(target));

    tex.desc.subDesc = subDesc;
    GL_CALL(glBindTexture(target, 0));

    return AXError::NoError();
}

AXError GLGraphicsBackend::DestroyTexture(TextureHandle& handle) {
    if (IsValidHandle(m_Textures, handle))
        return {"Invalid Handle"};
    auto& tex = m_Textures[handle.index];
    GL_CALL(glDeleteTextures(1, &tex.id));
    tex.id = 0;
    PostDeleteHandle(tex, handle, m_FreeTextures);
    return AXError::NoError();
}

ExResult<FramebufferHandle> GLGraphicsBackend::CreateFramebuffer(const FramebufferDesc& desc) {
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
                "Invalid Texture Handle at desc.colorAttachments[" +
                std::to_string(i) + "]"
            };
        }

        auto& tex = m_Textures[texHandle.index];
        const AttachmentDesc& rpAtt = rpDesc.colorAttachments.data[i];

        if (tex.desc.format != rpAtt.format) {
            return AXError{
                "Color attachment format mismatch at index " +
                std::to_string(i)
            };
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

    GL_CALL(glGenFramebuffers(1, &fb.fbo));
    GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, fb.fbo));

    for (uint32_t i = 0; i < desc.colorAttachments.size; ++i) {
        auto& tex = m_Textures[desc.colorAttachments.data[i].index];

        GL_CALL(glFramebufferTexture2D(
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

        GL_CALL(glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            attachmentGLType,
            ToGLTarget(depthStencilTex.desc.type),
            depthStencilTex.id,
            0
        ));
    }

    GLenum status = GL_CALL(glCheckFramebufferStatus(GL_FRAMEBUFFER));
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        GL_CALL(glDeleteFramebuffers(1, &fb.fbo));
        GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));

        return AXError{"Framebuffer incomplete (GL error)"};
    }

    GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));

    fb.alive  = true;
    fb.hasDepth = hasDepth;
    fb.hasStencil = hasStencil;
    fb.width  = desc.width;
    fb.height = desc.height;
    fb.renderPass = rp;

    return FramebufferHandle{fb.index, fb.generation};
}

AXError GLGraphicsBackend::DestroyFramebuffer(FramebufferHandle& handle) {
    if (!IsValidHandle(m_Framebuffers, handle))
        return {"Invalid Handle"};

    GLFramebuffer& fb = m_Framebuffers[handle.index];

    GL_CALL(glDeleteFramebuffers(1, &fb.fbo));
    fb.fbo = 0;
    PostDeleteHandle(fb, handle, m_FreeFramebuffers);

    return AXError::NoError();
}

ExResult<ShaderHandle> GLGraphicsBackend::CreateProgram(const ShaderDesc& desc) {
    if (desc.entryPoints.size > 1) {
        for (size_t i{0}; i < desc.entryPoints.size; i++) {
            if (desc.entryPoints.data[i].stage == ShaderStage::Compute) {
                return AXError{"A compute pipeline must have only one single compute shader entry"};
            }
        }
    }

    auto& program = ReserveHandle(m_Programs, m_FreePrograms);
    program.id = GL_CALL(glCreateProgram()); 

    int major = 0, minor = 0;
    GL_CALL(glGetIntegerv(GL_MAJOR_VERSION, &major));
    GL_CALL(glGetIntegerv(GL_MINOR_VERSION, &minor));

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
        glDeleteProgram(program.id);
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

        GLuint shader = GL_CALL(glCreateShader(glStage));
        GL_CALL(glShaderSource(shader, 1, &glsl, nullptr));
        GL_CALL(glCompileShader(shader));

        GLint state;
        GL_CALL(glGetShaderiv(shader, GL_COMPILE_STATUS, &state));
        if (!state) {
            std::vector<char> log(4096);
            GLsizei length;
            GL_CALL(glGetShaderInfoLog(shader, log.size(), &length, log.data()));
    
            std::stringstream error_msg{};
            std::string start = "Failed to load GLSL entrypoint \"" + glName + "\"\n\n";
            error_msg.write(start.c_str(), start.size());
            error_msg.write(log.data(), length);
    
            GL_CALL(glDeleteShader(shader));
            GL_CALL(glDeleteProgram(program.id));
            program.id = 0;
            return {error_msg.str()};
        }

        GL_CALL(glAttachShader(program.id, shader));
        GL_CALL(glDeleteShader(shader));
    }

    GL_CALL(glLinkProgram(program.id));

    GLint linkedState;
    GL_CALL(glGetProgramiv(program.id, GL_LINK_STATUS, &linkedState));
    if (!linkedState) {
        std::vector<char> log(4096);
        GLsizei length;
        GL_CALL(glGetProgramInfoLog(program.id, log.size(), &length, log.data()));
        std::stringstream error_msg{};
        std::string start = "Failed to link GLSL shaders\n\n";
        error_msg.write(start.c_str(), start.size());
        error_msg.write(log.data(), length);
        GL_CALL(glDeleteProgram(program.id));
        program.id = 0;
        return {error_msg.str()};
    }
    program.alive = true;

    return ShaderHandle{program.index, program.generation};
}

AXError GLGraphicsBackend::DestroyProgram(ShaderHandle& handle) {
    if (!IsValidHandle(m_Programs, handle))
        return {"Invalid Handle"};

    GLProgram& program = m_Programs[handle.index];

    GL_CALL(glDeleteProgram(program.id));
    program.id = 0;
    PostDeleteHandle(program, handle, m_FreePrograms);
    return AXError::NoError();
}

ExResult<RenderPipelineHandle> GLGraphicsBackend::CreateRenderPipeline(const RenderPipelineDesc& desc) {
    if (!IsValidHandle(m_Programs, desc.shader))
        return AXError("Invalid Pipeline Shader");

    auto& pipeline = ReserveHandle(m_RenderPipelines, m_FreeRenderPipelines);
    
    GL_CALL(glGenVertexArrays(1, &pipeline.vao));
    GL_CALL(glBindVertexArray(pipeline.vao));
    
    auto& attrSpan = desc.layout.attributes;
    for (size_t i{0}; i < attrSpan.size; i++) {
        auto& attr = attrSpan.data[i];
        GL_CALL(glEnableVertexAttribArray(attr.location));

        switch (attr.typeDesc._class) {
            case VertexAttributeClass::Float:
                GL_CALL(glVertexAttribPointer(
                    attr.location,
                    attr.componentCount,
                    ToGLType(attr.typeDesc.type),
                    attr.normalized,
                    desc.layout.stride,
                    (void*)(uintptr_t)attr.offset
                ));
            break;
            case VertexAttributeClass::Int:
                GL_CALL(glVertexAttribIPointer(
                    attr.location,
                    attr.componentCount,
                    ToGLType(attr.typeDesc.type),
                    desc.layout.stride,
                    (void*)(uintptr_t)attr.offset
                ));
            break;
            case VertexAttributeClass::Double:
                if (!m_Capabilities.Has(GraphicsCapEnum::LongPointers) || glVertexAttribLPointer == nullptr) {
                    return AXError("Invalid vertex attribute type: Host GPU doesn't support 64 bit pointers");
                }
                GL_CALL(glVertexAttribLPointer(
                    attr.location,
                    attr.componentCount,
                    ToGLType(attr.typeDesc.type),
                    desc.layout.stride,
                    (void*)(uintptr_t)attr.offset
                ));
            break;
        }
        GL_CALL(glVertexAttribDivisor(attr.location, attr.divisor));
    }
    GL_CALL(glBindVertexArray(0));

    pipeline.desc = desc;
    pipeline.program = m_Programs[desc.shader.index];
    pipeline.vaoLayout = desc.layout;
    pipeline.alive = true;

    return RenderPipelineHandle{pipeline.index, pipeline.generation};
}

AXError GLGraphicsBackend::DestroyRenderPipeline(RenderPipelineHandle& handle) {
    if (!IsValidHandle(m_RenderPipelines, handle)) 
        return {"Invalid Handle"};
    auto& pipeline = m_RenderPipelines[handle.index];
    GL_CALL(glDeleteVertexArrays(1, &pipeline.vao));
    pipeline.vao = 0;
    PostDeleteHandle(pipeline, handle, m_FreeRenderPipelines);
    return AXError::NoError();
}

ExResult<RenderPassHandle> GLGraphicsBackend::CreateRenderPass(const RenderPassDesc& desc) {
    auto& pass = ReserveHandle(m_RenderPasses, m_FreeRenderPasses);
    pass.desc = desc;
    pass.alive = true;
    return RenderPassHandle{pass.index, pass.generation};
}

AXError GLGraphicsBackend::DestroyRenderPass(RenderPassHandle& handle) {
    if (!IsValidHandle(m_RenderPasses, handle))
        return {"Invalid Handle"};
    PostDeleteHandle(m_RenderPasses[handle.index], handle, m_FreeRenderPasses);
    return AXError::NoError();
}

AXError GLGraphicsBackend::Dispatch(ICommandList& cmd, uint32_t x, uint32_t y, uint32_t z) {
    if (x > m_Capabilities.maxWorkGroupCount[0] ||
        y > m_Capabilities.maxWorkGroupCount[1] ||
        z > m_Capabilities.maxWorkGroupCount[2]
    ) return AXError{"Dispatch exceeds max workGroup count"};

    if (!m_CurrentComputePipeline.bound) {
        return AXError{"Bad commands, requested Compute Dispatch, but no Compute Pipeline bound!"};
    }
    Execute(static_cast<GLCommandList&>(cmd));
    GL_CALL(glDispatchCompute(x, y, z));
    return AXError::NoError();
}

AXError GLGraphicsBackend::BindResources(ICommandList& cmd, Span<Binding> bindings) {
    auto& glcmd = static_cast<GLCommandList&>(cmd);

}

AXError GLGraphicsBackend::Barrier(ICommandList&, Span<ResourceTransition> transitions) {
    GLenum bits = 0;

    for (size_t i{0}; i < transitions.size; i++) {
        bits |= ToGLBarrierBit(transitions.data[i].newState);
    }
    if (bits != 0) {
        GL_CALL(glMemoryBarrier(bits));
    }
    return AXError::NoError();
}

AXError GLGraphicsBackend::Execute(const GLCommandList& cmd) {
    for (const auto& c : cmd.Commands()) {
        if (c.type >= GLCommandType::BindBuffer) {
            if (!m_CurrentRenderPass.bound)
                return {"GLCommand::Execute \"GLCommandType::BindBuffer/DrawXYZ\" Failed: No RenderPass bound, use BeginRenderPass()"};
            if (!m_CurrentRenderPipeline.bound)
                return {"GLCommand::Execute \"GLCommandType::BindBuffer/DrawXYZ\" Failed: No RenderPipline bound, use BindRenderPipeline()"};

            if (!IsValidHandle(m_RenderPasses, m_CurrentRenderPass.handle))
                return {"GLCommand::Execute \"GLCommandType::BindBuffer/DrawXYZ\" Failed: Invalid RenderPass bound!"};
            if (!IsValidHandle(m_RenderPipelines, m_CurrentRenderPipeline.handle))
                return {"GLCommand::Execute \"GLCommandType::BindBuffer/DrawXYZ\" Failed: Invalid RenderPipline bound!"};
        }

        switch (c.type) {
            case GLCommandType::BeginRenderPass: {
                uint32_t rpindex = c.args[0], rpgeneration = c.args[1];
                if (!IsValidHandle(m_RenderPasses, rpindex, rpgeneration))
                    return {"GLCommand::Execute \"BeginRenderPass\" Failed: Invalid RenderPass handle"};

                uint32_t fbindex = c.args[2], fbgeneration = c.args[3];
                if (!IsValidHandle(m_RenderPipelines, fbindex, fbgeneration))
                    return {"GLCommand::Execute \"BeginRenderPass\" Failed: Invalid Framebuffer handle"};

                GLRenderPass& pass = m_RenderPasses[rpindex];
                GLFramebuffer& fb  = m_Framebuffers[fbindex];

                GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, fb.fbo));

                // Set draw buffers
                std::vector<GLenum> drawBuffers(pass.desc.colorAttachments.size);
                for (size_t i = 0; i < pass.desc.colorAttachments.size; ++i)
                    drawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;

                GL_CALL(glDrawBuffers((GLsizei)drawBuffers.size(), drawBuffers.data()));

                // Clear attachments based on LoadOp
                for (size_t i = 0; i < pass.desc.colorAttachments.size; ++i) {
                    auto& att = pass.desc.colorAttachments.data[i];

                    if (att.load == LoadOp::Clear) {
                        GL_CALL(glClearBufferfv(GL_COLOR, (GLint)i, (const GLfloat*)(&c.args[4])));
                    }
                }
                if (fb.hasDepth) {
                    auto& depthStencil = pass.desc.depthStencilAttachment;

                    if (depthStencil.load == LoadOp::Clear) {
                        GL_CALL(glClearBufferfv(GL_DEPTH, 0, (const GLfloat*)(&c.args[7])));
                    }
                }
                if (fb.hasStencil) {
                    auto& depthStencil = pass.desc.depthStencilAttachment;

                    if (depthStencil.load == LoadOp::Clear) {
                        GL_CALL(glClearBufferiv(GL_STENCIL, 0, (const GLint*)(&c.args[8])));
                    }
                }

                m_CurrentRenderPass.Bind({rpindex, rpgeneration});
                break;
            }
            case GLCommandType::EndRenderPass: {
                uint32_t rpindex = c.args[0], rpgeneration = c.args[1];
                if (!IsValidHandle(m_RenderPasses, rpindex, rpgeneration))
                    return {"GLCommand::Execute \"GLCommandType::EndRenderPass\" Failed: Invalid RenderPass handle"};

                GLRenderPass& pass = m_RenderPasses[rpindex];

                std::vector<GLenum> invalidateAttachments;

                for (size_t i = 0; i < pass.desc.colorAttachments.size; ++i) {
                    if (pass.desc.colorAttachments.data[i].store == StoreOp::Discard)
                        invalidateAttachments.push_back(GL_COLOR_ATTACHMENT0 + i);
                }

                if (pass.desc.depthStencilAttachment.store == StoreOp::Discard)
                    invalidateAttachments.push_back(GL_DEPTH_ATTACHMENT);

                if (!invalidateAttachments.empty()) {
                    GL_CALL(glInvalidateFramebuffer(
                        GL_FRAMEBUFFER,
                        (GLsizei)invalidateAttachments.size(),
                        invalidateAttachments.data()
                    ));
                }

                GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
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
                auto& desc = pipeline.desc;

                if (
                    m_CurrentRenderPipeline.bound &&
                    m_CurrentRenderPipeline.handle.index == index &&
                    m_CurrentRenderPipeline.handle.generation == generation
                ) {
                    break;
                }

                m_CurrentRenderPipeline.Bind({index, generation});

                if (m_CurrentState.program != pipeline.program.id) {
                    GL_CALL(glUseProgram(pipeline.program.id));
                    m_CurrentState.program = pipeline.program.id;
                }

                bool wantCull = desc.raster.cull != CullMode::None;

                if (m_CurrentState.cullEnabled != wantCull) {
                    m_CurrentState.cullEnabled = wantCull;
                    wantCull ? GL_CALL(glEnable(GL_CULL_FACE))
                            : GL_CALL(glDisable(GL_CULL_FACE));
                }

                if (wantCull) {
                    GLenum face = desc.raster.cull == CullMode::Front ? GL_FRONT : GL_BACK;

                    if (m_CurrentState.cullFace != face) {
                        GL_CALL(glCullFace(face));
                        m_CurrentState.cullFace = face;
                    }
                }

                GLenum front = desc.raster.frontFace == FrontFace::Clockwise ? GL_CW : GL_CCW;

                if (m_CurrentState.frontFace != front) {
                    GL_CALL(glFrontFace(front));
                    m_CurrentState.frontFace = front;
                }

                GLenum poly = desc.raster.fill == FillMode::Wireframe ? GL_LINE : GL_FILL;

                if (m_CurrentState.polygonMode != poly) {
                    GL_CALL(glPolygonMode(GL_FRONT_AND_BACK, poly));
                    m_CurrentState.polygonMode = poly;
                }

                if (m_CurrentState.depthTest != desc.depth.depthTest) {
                    m_CurrentState.depthTest = desc.depth.depthTest;
                    desc.depth.depthTest ? GL_CALL(glEnable(GL_DEPTH_TEST))
                                        : GL_CALL(glDisable(GL_DEPTH_TEST));
                }

                if (m_CurrentState.depthWrite != desc.depth.depthWrite) {
                    GL_CALL(glDepthMask(desc.depth.depthWrite ? GL_TRUE : GL_FALSE));
                    m_CurrentState.depthWrite = desc.depth.depthWrite;
                }

                GLenum depthFunc = ToGLCompare(desc.depth.depthCompare);
                if (m_CurrentState.depthFunc != depthFunc) {
                    GL_CALL(glDepthFunc(depthFunc));
                    m_CurrentState.depthFunc = depthFunc;
                }

                if (m_CurrentState.stencilTest != desc.depth.stencilTest) {
                    m_CurrentState.stencilTest = desc.depth.stencilTest;
                    desc.depth.stencilTest ? GL_CALL(glEnable(GL_STENCIL_TEST))
                                        : GL_CALL(glDisable(GL_STENCIL_TEST));
                }

                if (desc.depth.stencilTest) {
                    GL_CALL(glStencilFuncSeparate(
                        GL_FRONT,
                        ToGLCompare(desc.depth.stencilFront.compare),
                        desc.depth.stencilFront.reference,
                        desc.depth.stencilFront.compareMask));

                    GL_CALL(glStencilOpSeparate(
                        GL_FRONT,
                        ToGLStencilOp(desc.depth.stencilFront.failOp),
                        ToGLStencilOp(desc.depth.stencilFront.depthFailOp),
                        ToGLStencilOp(desc.depth.stencilFront.passOp)));

                    GL_CALL(glStencilMaskSeparate(
                        GL_FRONT,
                        desc.depth.stencilFront.writeMask));

                    GL_CALL(glStencilFuncSeparate(
                        GL_BACK,
                        ToGLCompare(desc.depth.stencilBack.compare),
                        desc.depth.stencilBack.reference,
                        desc.depth.stencilBack.compareMask));

                    GL_CALL(glStencilOpSeparate(
                        GL_BACK,
                        ToGLStencilOp(desc.depth.stencilBack.failOp),
                        ToGLStencilOp(desc.depth.stencilBack.depthFailOp),
                        ToGLStencilOp(desc.depth.stencilBack.passOp)));

                    GL_CALL(glStencilMaskSeparate(
                        GL_BACK,
                        desc.depth.stencilBack.writeMask));
                }

                if (m_CurrentState.blendEnabled != desc.blend.enabled) {
                    m_CurrentState.blendEnabled = desc.blend.enabled;
                    desc.blend.enabled ? GL_CALL(glEnable(GL_BLEND))
                                    : GL_CALL(glDisable(GL_BLEND));
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
                        GL_CALL(glBlendFuncSeparate(srcC, dstC, srcA, dstA));
                        m_CurrentState.srcColor = srcC;
                        m_CurrentState.dstColor = dstC;
                        m_CurrentState.srcAlpha = srcA;
                        m_CurrentState.dstAlpha = dstA;
                    }

                    if (
                        m_CurrentState.blendColorOp != opC ||
                        m_CurrentState.blendAlphaOp != opA
                    ) {
                        GL_CALL(glBlendEquationSeparate(opC, opA));
                        m_CurrentState.blendColorOp = opC;
                        m_CurrentState.blendAlphaOp = opA;
                    }
                }

                if (m_CurrentState.vao != pipeline.vao) {
                    GL_CALL(glBindVertexArray(pipeline.vao));
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
            case GLCommandType::BindBuffer: {
                uint32_t index = c.args[0], generation = c.args[1];
                if (!IsValidHandle(m_Buffers, index, generation))
                    return {"GLCommand::Execute \"GLCommandType::BindBuffer\" Failed: Invalid buffer handle"};
                auto& buff = m_Buffers[index];
                if (!m_CurrentRenderPipeline.bound)
                    return {"GLCommand::Execute \"GLCommandType::BindBuffer\" Failed: No RenderPipline bound, use BindRenderPipeline()"};
                if (!IsValidHandle(m_RenderPipelines, m_CurrentRenderPipeline.handle))
                    return {"GLCommand::Execute \"GLCommandType::BindBuffer\" Failed: Invalid RenderPipeline bound!"};
                auto& pipelineHandle = m_CurrentRenderPipeline.handle;

                switch (buff.usage) {
                    case BufferUsage::Vertex: {
                        auto& pipeline = m_RenderPipelines[pipelineHandle.index];
                        if (pipeline.vbo != buff.id) {
                            pipeline.vbo = buff.id;
                            glBindBuffer(GL_ARRAY_BUFFER, buff.id);
                        }
                    }
                    break;
                    case BufferUsage::Index: {
                        auto& pipeline = m_RenderPipelines[pipelineHandle.index];
                        if (pipeline.ebo != buff.id) {
                            pipeline.ebo = buff.id;
                            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buff.id);
                        }
                    }
                    break;
                    case BufferUsage::Uniform:
                        if (static_cast<int32_t>(c.args[2]) >= m_Capabilities.maxUBOSize)
                            return {"GLCommand::Execute \"GLCommandType::BindBuffer\" Failed: Uniform buffer slot out of range"};
                        glBindBufferBase(GL_UNIFORM_BUFFER, buff.slot, buff.id);
                    break;
                    case BufferUsage::Storage:
                        if (static_cast<int32_t>(c.args[2]) >= m_Capabilities.maxUBOSize)
                            return {"GLCommand::Execute \"GLCommandType::BindBuffer\" Failed: Uniform buffer slot out of range or Unsupported on this device"};
                        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, buff.slot, buff.id);
                    break;
                    case BufferUsage::Indirect:
                        if (!m_Capabilities.Has(GraphicsCapEnum::IndirectDraw))
                            return {"GLCommand::Execute \"GLCommandType::BindBuffer\" Failed: Indirect Rendering is Unsupported on this device"};
                        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buff.id);
                    break;
                    case BufferUsage::Staging:
                        return {"GLCommand::Execute \"GLCommandType::BindBuffer\" Failed: Staging buffers are used only for CPU->GPU copies, never bound."};
                    break;
                }
                break;
            }
            // c.args[0] ~ TRIANGLES, LINES, DOTS
            // c.args[1] ~ count
            case GLCommandType::Draw: {
                GL_CALL(glDrawArrays(c.args[0], c.args[2], c.args[1]));
                //glDrawArraysInstanced();
                //glDrawElementsInstanced();
                //glMultiDrawArrays();
                //glMultiDrawElements();
                //glMultiDrawArraysIndirect();
                //glMultiDrawElementsIndirect();
                break;
            }
            case GLCommandType::DrawIndexed: {
                GL_CALL(glDrawElements(c.args[0], c.args[1], GL_UNSIGNED_INT, nullptr));
                break;
            }
            case GLCommandType::DrawIndirect: {
                GL_CALL(glDrawArraysIndirect(c.args[0], nullptr));
                break;
            }
            case GLCommandType::DrawIndirectIndexed: {
                GL_CALL(glDrawElementsIndirect(c.args[0], GL_UNSIGNED_INT, nullptr));
                break;
            }
        }
    }
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

        case VertexAttributeType::Float16:  return GL_HALF_FLOAT; // TODO: CRITICAL!! Verify extensions!!!
        case VertexAttributeType::Float32:  return GL_FLOAT;
        case VertexAttributeType::Float64:  return GL_DOUBLE;

        default:
            assert(false && "Unsupported vertex attribute type");
            return GL_INVALID_ENUM;
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

} // namespace axle::gfx
#endif
