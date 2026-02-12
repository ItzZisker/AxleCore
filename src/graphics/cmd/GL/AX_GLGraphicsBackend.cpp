#ifdef __AX_GRAPHICS_GL__
#include "axle/graphics/cmd/GL/AX_GLGraphicsBackend.hpp"

#include "axle/graphics/AX_Graphics.hpp"
#include "axle/utils/AX_Expected.hpp"
#include "axle/utils/AX_Types.hpp"

#include <slang.h>
#include <slang-com-ptr.h>
#include <slang-com-helper.h>

#include <glad/glad.h>

#include <sstream>
#include <string>
#include <cstring>
#include <vector>
#include <unordered_set>

using namespace axle::utils;

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
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);

    std::unordered_set<std::string> extensions;

    GLint extn = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &extn);
    for (GLint i = 0; i < extn; i++) {
        extensions.insert((const char*)glGetStringi(GL_EXTENSIONS, i));
    }
    auto HasExt = [&](const char* ext) {
        return extensions.contains(ext);
    };

    result.caps[(int)GraphicsCapEnum::GCapComputeShaders] =
        (major > 4 || (major == 4 && minor >= 3)) ||
        HasExt("GL_ARB_compute_shader");

    result.caps[(int)GraphicsCapEnum::GCapShaderStorageBuffers] =
        (major > 4 || (major == 4 && minor >= 3)) ||
        HasExt("GL_ARB_shader_storage_buffer_object");

    result.caps[(int)GraphicsCapEnum::GCapIndirectDraw] =
        (major >= 4) || HasExt("GL_ARB_draw_indirect");

    result.caps[(int)GraphicsCapEnum::GCapGeometryShaders] =
        (major > 3 || (major == 3 && minor >= 2)) ||
        HasExt("GL_ARB_geometry_shader4");

    result.caps[(int)GraphicsCapEnum::GCapTessellation] =
        (major > 4 || (major == 4 && minor >= 0)) ||
        HasExt("GL_ARB_tessellation_shader");
        
    result.caps[(int)GraphicsCapEnum::GCapMultiDrawIndirect] =
        (major > 4 || (major == 4 && minor >= 3)) ||
        HasExt("GL_ARB_multi_draw_indirect");
        
    result.caps[(int)GraphicsCapEnum::GCapTextureArray] =
        (major >= 3) ||
        HasExt("GL_EXT_texture_array");

    result.caps[(int)GraphicsCapEnum::GCapSparseTextures] =
        (major > 4 || (major == 4 && minor >= 4)) ||
        HasExt("GL_ARB_sparse_texture");
    
    result.caps[(int)GraphicsCapEnum::GCapRayTracing] =
        HasExt("GL_NV_ray_tracing"); // NVIDIA-Only

    return result;
}

ExResult<BufferHandle> GLGraphicsBackend::CreateBuffer(const BufferDesc& desc) {
    auto [index, buff] = ReserveHandle(m_Buffers, m_FreeBuffers);
    
    GLuint id;
    glGenBuffers(1, &id);

    GLenum target = GL_ARRAY_BUFFER;
    if (desc.usage == BufferUsage::BuIndex) target = GL_ELEMENT_ARRAY_BUFFER;

    glBindBuffer(target, id);
    glBufferData(target, desc.size, nullptr,
        desc.access == BufferAccess::BLvlImmutable ? GL_STATIC_DRAW :
        desc.access == BufferAccess::BLvlDynamic ? GL_DYNAMIC_DRAW : GL_STREAM_DRAW);

    buff.id = id;
    buff.target = target;
    buff.alive = true;

    return BufferHandle{index, buff.generation};
}

AXError GLGraphicsBackend::UpdateBuffer(BufferHandle handle, size_t offset, size_t size, const void* data) {
    if (!IsValidHandle(m_Buffers, handle))
        return {"Invalid Handle"};
    auto& buff = m_Buffers[handle.index];
    glBindBuffer(buff.target, buff.id);
    glBufferSubData(buff.target, offset, size, data);
    return AXError::NoError();
}

AXError GLGraphicsBackend::DestroyBuffer(BufferHandle& handle) {
    if (!IsValidHandle(m_Buffers, handle))
        return {"Invalid Handle"};
    if (it != m_Buffers.end()) {
        h.generation = 0;
        glDeleteBuffers(1, &it->second.id);
        m_Buffers.erase(it);
    }
    return AXError::NoError();
}

TextureHandle GLGraphicsBackend::CreateTexture(const TextureDesc& desc) {
    GLuint id;
    
    GLenum target = TTypeGLTarget(desc.type);
    GLenum internalFormat = TFmtGLInternalFormat(desc.format);
    GLenum format = TFmtGLFormat(desc.format);
    GLenum type = TFmtGLType(desc.format);
    bool compressed = TFmtIsCompressed(desc.format);

    glGenTextures(1, &id);
    glBindTexture(target, id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // safest: 1, but slightly less optimal for some GPUs

    switch (desc.type) {
        case TextureType::TxTTex2D:
            if (compressed) {
                glCompressedTexImage2D(
                    target,
                    desc.subDesc.mipLevels,
                    format,
                    desc.width,
                    desc.height,
                    0,
                    (GLsizei) desc.pixelsByLayers.size(),
                    desc.pixelsByLayers.data()
                );
            } else {
                glTexImage2D(
                    target,
                    desc.subDesc.mipLevels,
                    internalFormat,
                    desc.width,
                    desc.height,
                    0,
                    format,
                    type,
                    desc.pixelsByLayers.data()
                );
            }
        break;
        case TextureType::TxTArray2D:
        case TextureType::TxTTex3D:
            if (compressed) {
                glCompressedTexImage3D(
                    target,
                    0,
                    format,
                    desc.width,
                    desc.height,
                    desc.layers,
                    0, 
                    (GLsizei) desc.pixelsByLayers.size(),
                    desc.pixelsByLayers.data()
                );
            } else {
                glTexImage3D(
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
                );
            }
        break;
        case TextureType::TxTCube:
            for (unsigned int i = 0; i < 6; i++) {
                if (compressed) {
                    glCompressedTexImage2D(
                        GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                        0,
                        format,
                        desc.width,
                        desc.height,
                        0,
                        (GLsizei) desc.pixelsByCubemap[i].size(),
                        desc.pixelsByCubemap[i].data()
                    );
                } else {
                    glTexImage2D(
                        GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                        0,
                        internalFormat,
                        desc.width,
                        desc.height,
                        0,
                        format,
                        type,
                        desc.pixelsByCubemap[i].data()
                    );
                }
            }
        break;
    }

    glTexParameteri(target, GL_TEXTURE_WRAP_S, TwGLWrap(desc.subDesc.wrapS));
    glTexParameteri(target, GL_TEXTURE_WRAP_T, TwGLWrap(desc.subDesc.wrapT));
    glTexParameteri(target, GL_TEXTURE_WRAP_R, TwGLWrap(desc.subDesc.wrapR));

    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, TfGLFilter(desc.subDesc.minFilter));
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, TfGLFilter(desc.subDesc.magFilter));

    if (desc.subDesc.wrapS == TextureWrap::TwClampToBorder ||
        desc.subDesc.wrapT == TextureWrap::TwClampToBorder ||
        desc.subDesc.wrapR == TextureWrap::TwClampToBorder)
        glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, &desc.subDesc.borderColor.r);

    if (desc.subDesc.generateMips && desc.subDesc.mipLevels == 0)
        glGenerateMipmap(target);

    TextureHandle h{static_cast<uint32_t>(m_Textures.size()), 1};
    m_Textures[h.index] = {id, desc};

    glBindTexture(target, 0);
    return h;
}

void GLGraphicsBackend::UpdateTexture(TextureHandle h, const TextureSubDesc& subDesc, const void* data) {
    auto p = m_Textures.find(h.index);
    if (p == m_Textures.end()) return;

    auto& tex = m_Textures[h.index];
    auto& desc = tex.desc;

    GLenum target = TTypeGLTarget(desc.type);
    GLenum format = TFmtGLFormat(desc.format);
    GLenum type = TFmtGLType(desc.format);
    bool compressed = TFmtIsCompressed(desc.format);

    glBindTexture(target, tex.id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // safest

    switch (desc.type) {
        case TextureType::TxTTex2D:
            if (compressed) {
                glCompressedTexSubImage2D(
                    target,
                    subDesc.mipLevels,
                    0, 0,
                    desc.width,
                    desc.height,
                    format,
                    desc.pixelsByLayers.size(),
                    data
                );
            } else {
                glTexSubImage2D(
                    target,
                    subDesc.mipLevels,
                    0, 0,
                    desc.width,
                    desc.height,
                    format,
                    type,
                    data
                );
            }
        break;
        case TextureType::TxTArray2D:
        case TextureType::TxTTex3D:
            if (compressed) {
                glCompressedTexSubImage3D(
                    target,
                    subDesc.mipLevels,
                    0, 0, 0,
                    desc.width,
                    desc.height,
                    desc.layers,
                    format,
                    desc.pixelsByLayers.size(),
                    data
                );
            } else {
                glTexSubImage3D(
                    target,
                    subDesc.mipLevels,
                    0, 0, 0,
                    desc.width,
                    desc.height,
                    desc.layers,
                    format,
                    type,
                    data
                );
            }
        break;
        case TextureType::TxTCube:
            for (unsigned int i = 0; i < 6; ++i) {
                if (compressed) {
                    glCompressedTexSubImage2D(
                        GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                        subDesc.mipLevels,
                        0, 0,
                        desc.width,
                        desc.height,
                        format,
                        (GLsizei) desc.pixelsByCubemap[i].size(),
                        data
                    );
                } else {
                    glTexSubImage2D(
                        GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                        subDesc.mipLevels,
                        0, 0,
                        desc.width,
                        desc.height,
                        format,
                        type,
                        data
                    );
                }
            }
        break;
    }

    glTexParameteri(target, GL_TEXTURE_WRAP_S, TwGLWrap(subDesc.wrapS));
    glTexParameteri(target, GL_TEXTURE_WRAP_T, TwGLWrap(subDesc.wrapT));
    glTexParameteri(target, GL_TEXTURE_WRAP_R, TwGLWrap(subDesc.wrapR));

    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, TfGLFilter(subDesc.minFilter));
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, TfGLFilter(subDesc.magFilter));

    if (subDesc.wrapS == TextureWrap::TwClampToBorder ||
        subDesc.wrapT == TextureWrap::TwClampToBorder ||
        subDesc.wrapR == TextureWrap::TwClampToBorder)
        glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, &subDesc.borderColor.r);

    if (subDesc.generateMips && subDesc.mipLevels == 0)
        glGenerateMipmap(target);

    tex.desc.subDesc = subDesc;
    glBindTexture(target, 0);
}

void GLGraphicsBackend::DestroyTexture(TextureHandle& h) {
    if (m_Textures.find(h.index) == m_Textures.end()) return;
    glDeleteTextures(1, &m_Textures[h.index].id);
    h.generation = 0;
    m_Textures.erase(h.index);
}

ExResult<FramebufferHandle> GLGraphicsBackend::CreateFramebuffer(const FramebufferDesc& desc) {
    auto [index, fb] = ReserveHandle(m_Framebuffers, m_FreeFramebuffers);

    glGenFramebuffers(1, &fb.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fb.fbo);

    // Attach color targets
    for (uint32_t i = 0; i < desc.colorAttachments.size; ++i) {
        auto& tex = m_Textures[desc.colorAttachments.data[i].index];
        glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0 + i,
            TTypeGLTarget(tex.desc.type),
            tex.id,
            0
        );
    }

    // Depth attachment
    if (desc.depthAttachment.IsValid()) {
        auto& depthTex = m_Textures[desc.depthAttachment.index];
        glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            GL_DEPTH_ATTACHMENT,
            TTypeGLTarget(depthTex.desc.type),
            depthTex.id,
            0
        );
    }

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        return AXError("Framebuffer incomplete");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    fb.alive = true;
    fb.width  = desc.width;
    fb.height = desc.height;

    return FramebufferHandle{ index, fb.generation };
}

AXError GLGraphicsBackend::DestroyFramebuffer(FramebufferHandle handle) {
    if (!IsValidHandle(m_Framebuffers, handle))
        return {"Invalid Handle"};

    GLFramebuffer& fb = m_Framebuffers[handle.index];

    glDeleteFramebuffers(1, &fb.fbo);
    fb.fbo = 0;

    if (fb.depthRbo) {
        glDeleteRenderbuffers(1, &fb.depthRbo);
        fb.depthRbo = 0;
    }
    PostDeleteHandle(fb, handle, m_FreeFramebuffers);

    return AXError::NoError();
}

ExResult<ShaderHandle> GLGraphicsBackend::CreateProgram(const ShaderDesc& desc) {
    auto [index, program] = ReserveHandle(m_Programs, m_FreePrograms);
    program.id = glCreateProgram(); 

    int major = 0, minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);

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
        GLenum glStage = StgGLStage(desc.entryPoints.data[i].stage);

        GLuint shader = glCreateShader(glStage);
        glShaderSource(shader, 1, &glsl, nullptr);
        glCompileShader(shader);

        GLint state;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &state);
        if (!state) {
            std::vector<char> log(4096);
            GLsizei length;
            glGetShaderInfoLog(shader, log.size(), &length, log.data());
    
            std::stringstream error_msg{};
            std::string start = "Failed to load GLSL entrypoint \"" + glName + "\"\n\n";
            error_msg.write(start.c_str(), start.size());
            error_msg.write(log.data(), length);
    
            glDeleteShader(shader);
            glDeleteProgram(program.id);
            program.id = 0;
            return {error_msg.str()};
        }

        glAttachShader(program.id, shader);
        glDeleteShader(shader);
    }

    glLinkProgram(program.id);

    GLint linkedState;
    glGetProgramiv(program.id, GL_LINK_STATUS, &linkedState);
    if (!linkedState) {
        std::vector<char> log(4096);
        GLsizei length;
        glGetProgramInfoLog(program.id, log.size(), &length, log.data());
        std::stringstream error_msg{};
        std::string start = "Failed to link GLSL shaders\n\n";
        error_msg.write(start.c_str(), start.size());
        error_msg.write(log.data(), length);
        glDeleteProgram(program.id);
        program.id = 0;
        return {error_msg.str()};
    }
    program.alive = true;


    return ShaderHandle{index, program.generation};
}

AXError GLGraphicsBackend::DestroyProgram(ShaderHandle& handle) {
    if (!IsValidHandle(m_Programs, handle))
        return {"Invalid Handle"};

    GLProgram& program = m_Programs[handle.index];

    glDeleteProgram(program.id);
    program.id = 0;
    PostDeleteHandle(program, handle, m_FreePrograms);
    return AXError::NoError();
}

ExResult<PipelineHandle> GLGraphicsBackend::CreatePipeline(const PipelineDesc& desc) {
    auto [index, pipeline] = ReserveHandle(m_Pipelines, m_FreePipelines);
    pipeline.alive = IsValidHandle(m_Programs, desc.shader);
    if (!pipeline.alive) return AXError("Invalid Pipeline Shader");
    return PipelineHandle{index, pipeline.generation};
}

AXError GLGraphicsBackend::DestroyPipeline(PipelineHandle& handle) {
    if (!IsValidHandle(m_Pipelines, handle)) return {"Invalid Handle"};
    else PostDeleteHandle(m_Pipelines[handle.index], handle, m_FreePipelines);
}

ExResult<VertexInputHandle> GLGraphicsBackend::CreateVertexInput(const VertexInputDesc& desc) {
    auto [index, vi] = ReserveHandle(m_VertexInputs, m_FreeVertexInputs);

    glGenVertexArrays(1, &vi.id);
    glBindVertexArray(vi.id);

    for (size_t i{0}; i < desc.vertexBuffers.size; ++i) {
        auto& buffHandle = desc.vertexBuffers.data[i];
        if (!IsValidHandle(m_Buffers, buffHandle)) {
            glDeleteVertexArrays(1, &vi.id);
            vi.id = 0;
            return AXError("Invalid VertexBuffer handle at index=" + std::to_string(i));
        }
        glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[buffHandle.index].id);

        auto& attrSpan = desc.layout.attributes;
        for (size_t i{0}; i < attrSpan.size; i++) {
            auto& attr = attrSpan.data[i];
            glEnableVertexAttribArray(attr.location);

            switch (attr.typeDesc._class) {
                case VertexAttributeClass::VAcFloat:
                    glVertexAttribPointer(
                        attr.location,
                        attr.componentCount,
                        AttrTGLType(attr.typeDesc.type),
                        attr.normalized,
                        desc.layout.stride,
                        (void*)(uintptr_t)attr.offset
                    );
                break;
                case VertexAttributeClass::VAcInt:
                    glVertexAttribIPointer(
                        attr.location,
                        attr.componentCount,
                        AttrTGLType(attr.typeDesc.type),
                        desc.layout.stride,
                        (void*)(uintptr_t)attr.offset
                    );
                break;
                case VertexAttributeClass::VAcDouble:
                    if (m_Capabilities glVertexAttribLPointer == nullptr) {

                    }
                    glVertexAttribLPointer(
                        attr.location,
                        attr.componentCount,
                        AttrTGLType(attr.typeDesc.type),
                        desc.layout.stride,
                        (void*)(uintptr_t)attr.offset
                    );
                break;
            }
            glVertexAttribDivisor(attr.location, attr.divisor);
        }
    }

    if (desc.indexed) {
        if (IsValidHandle(m_Buffers, desc.indexBuffer)) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Buffers[desc.indexBuffer.index].id);
        } else {
            return AXError("Invalid IndexBuffer");
        }
    }
    glBindVertexArray(0);

    vi.desc = desc;
    vi.alive = true;

    return VertexInputHandle{index, vi.generation};
}

void GLGraphicsBackend::DestroyVertexInput(VertexInputHandle& handle) {
    if (m_VertexInputs.find(handle.index) == m_VertexInputs.end())
        return;
    GLVertexInput& vi = m_VertexInputs[handle.index];
    glDeleteVertexArrays(1, &vi.id);
    vi.id = 0;
    handle.generation = 0;
}

RenderPassHandle GLGraphicsBackend::CreateRenderPass(const RenderPassDesc&) {
    return RenderPassHandle{0, 1}; // OpenGL implicit
}

void GLGraphicsBackend::Dispatch(ICommandList&, uint32_t x, uint32_t y, uint32_t z) {
    //glDispatchCompute(x, y, z);
}

void GLGraphicsBackend::BindResources(ICommandList& cmd, Span<Binding> bindings) {

}

void GLGraphicsBackend::Execute(const GLCommandList& cmd) {
    for (const auto& c : cmd.Commands()) {
        switch (c.type) {
            case GLCommandType::BindPipeline:
                glUseProgram(m_Pipelines[c.args[0]].program);
            break;
            case GLCommandType::BindVertexBuffer:
                glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[c.args[0]].id);
            break;
            case GLCommandType::BindIndexBuffer:
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Buffers[c.args[0]].id);
            break;
            case GLCommandType::Draw:
                glDrawArrays(GL_TRIANGLES, c.args[1], c.args[0]);
            break;
            case GLCommandType::DrawIndexed:
                glDrawElements(GL_TRIANGLES, c.args[0], GL_UNSIGNED_INT, nullptr);
            break;
            default: break;
        }
    }
}

GLenum TwGLWrap(TextureWrap wrap) {
    switch (wrap) {
        case TextureWrap::TwRepeat:         return GL_REPEAT;
        case TextureWrap::TwMirrorRepeat:   return GL_MIRRORED_REPEAT;
        case TextureWrap::TwClampToEdge:    return GL_CLAMP_TO_EDGE;
        case TextureWrap::TwClampToBorder:  return GL_CLAMP_TO_BORDER;
        default: return GL_NONE;
    }
}

GLenum TfGLFilter(TextureFilter filter, bool mipmap) {
    switch (filter) {
        case TextureFilter::TfNearest:
            return mipmap ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST;
        case TextureFilter::TfLinear:
            return mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
        default: return GL_NONE;
    }
}

GLenum TFmtGLInternalFormat(TextureFormat fmt) {
    switch (fmt) {
        // --- 8-bit ---
        case TextureFormat::TxFmtR8_UNORM:      return GL_R8;
        case TextureFormat::TxFmtR8_SNORM:      return GL_R8_SNORM;
        case TextureFormat::TxFmtR8_UINT:       return GL_R8UI;
        case TextureFormat::TxFmtR8_SINT:       return GL_R8I;

        case TextureFormat::TxFmtRG8_UNORM:     return GL_RG8;
        case TextureFormat::TxFmtRG8_SNORM:     return GL_RG8_SNORM;
        case TextureFormat::TxFmtRG8_UINT:      return GL_RG8UI;
        case TextureFormat::TxFmtRG8_SINT:      return GL_RG8I;

        case TextureFormat::TxFmtRGBA8_UNORM:   return GL_RGBA8;
        case TextureFormat::TxFmtRGBA8_SNORM:   return GL_RGBA8_SNORM;
        case TextureFormat::TxFmtRGBA8_UINT:    return GL_RGBA8UI;
        case TextureFormat::TxFmtRGBA8_SINT:    return GL_RGBA8I;

        case TextureFormat::TxFmtBGRA8_UNORM:   return GL_RGBA8;
        case TextureFormat::TxFmtRGBA8_SRGB:    return GL_SRGB8_ALPHA8;

        // --- 16-bit ---
        case TextureFormat::TxFmtR16_UNORM:     return GL_R16;
        case TextureFormat::TxFmtR16_SNORM:     return GL_R16_SNORM;
        case TextureFormat::TxFmtR16_UINT:      return GL_R16UI;
        case TextureFormat::TxFmtR16_SINT:      return GL_R16I;
        case TextureFormat::TxFmtR16_FLOAT:     return GL_R16F;

        case TextureFormat::TxFmtRG16_UNORM:    return GL_RG16;
        case TextureFormat::TxFmtRG16_SNORM:    return GL_RG16_SNORM;
        case TextureFormat::TxFmtRG16_UINT:     return GL_RG16UI;
        case TextureFormat::TxFmtRG16_SINT:     return GL_RG16I;
        case TextureFormat::TxFmtRG16_FLOAT:    return GL_RG16F;

        case TextureFormat::TxFmtRGBA16_UNORM:  return GL_RGBA16;
        case TextureFormat::TxFmtRGBA16_SNORM:  return GL_RGBA16_SNORM;
        case TextureFormat::TxFmtRGBA16_UINT:   return GL_RGBA16UI;
        case TextureFormat::TxFmtRGBA16_SINT:   return GL_RGBA16I;
        case TextureFormat::TxFmtRGBA16_FLOAT:  return GL_RGBA16F;

        // --- 32-bit ---
        case TextureFormat::TxFmtR32_UINT:      return GL_R32UI;
        case TextureFormat::TxFmtR32_SINT:      return GL_R32I;
        case TextureFormat::TxFmtR32_FLOAT:     return GL_R32F;

        case TextureFormat::TxFmtRG32_UINT:     return GL_RG32UI;
        case TextureFormat::TxFmtRG32_SINT:     return GL_RG32I;
        case TextureFormat::TxFmtRG32_FLOAT:    return GL_RG32F;

        case TextureFormat::TxFmtRGB32_FLOAT:   return GL_RGB32F;
        case TextureFormat::TxFmtRGBA32_FLOAT:  return GL_RGBA32F;

        // --- Packed ---
        case TextureFormat::TxFmtRGB10A2_UNORM: return GL_RGB10_A2;
        case TextureFormat::TxFmtRG11B10_FLOAT: return GL_R11F_G11F_B10F;

        // --- Depth / Stencil ---
        case TextureFormat::TxFmtD16_UNORM:           return GL_DEPTH_COMPONENT16;
        case TextureFormat::TxFmtD24_UNORM_S8_UINT:   return GL_DEPTH24_STENCIL8;
        case TextureFormat::TxFmtD32_FLOAT:           return GL_DEPTH_COMPONENT32F;
        case TextureFormat::TxFmtD32_FLOAT_S8_UINT:   return GL_DEPTH32F_STENCIL8;

        // --- BC (S3TC) ---
        case TextureFormat::TxFmtBC1_UNORM:     return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        case TextureFormat::TxFmtBC1_SRGB:      return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
        case TextureFormat::TxFmtBC3_UNORM:     return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        case TextureFormat::TxFmtBC3_SRGB:      return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
        case TextureFormat::TxFmtBC4_UNORM:     return GL_COMPRESSED_RED_RGTC1;
        case TextureFormat::TxFmtBC4_SNORM:     return GL_COMPRESSED_SIGNED_RED_RGTC1;
        case TextureFormat::TxFmtBC5_UNORM:     return GL_COMPRESSED_RG_RGTC2;
        case TextureFormat::TxFmtBC5_SNORM:     return GL_COMPRESSED_SIGNED_RG_RGTC2;
        case TextureFormat::TxFmtBC6H_UFLOAT:   return GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT;
        case TextureFormat::TxFmtBC6H_SFLOAT:   return GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT;
        case TextureFormat::TxFmtBC7_UNORM:     return GL_COMPRESSED_RGBA_BPTC_UNORM;
        case TextureFormat::TxFmtBC7_SRGB:      return GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM;

        // --- ASTC (GLES mobile) ---
        case TextureFormat::TxFmtASTC_4x4_UNORM: return GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
        case TextureFormat::TxFmtASTC_4x4_SRGB:  return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR;
        case TextureFormat::TxFmtASTC_6x6_UNORM: return GL_COMPRESSED_RGBA_ASTC_6x6_KHR;
        case TextureFormat::TxFmtASTC_6x6_SRGB:  return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR;
        case TextureFormat::TxFmtASTC_8x8_UNORM: return GL_COMPRESSED_RGBA_ASTC_8x8_KHR;
        case TextureFormat::TxFmtASTC_8x8_SRGB:  return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR;

        default: return GL_NONE;
    }
}

GLenum TFmtGLFormat(TextureFormat fmt) {
    switch (fmt) {
        case TextureFormat::TxFmtR8_UNORM:
        case TextureFormat::TxFmtR8_SNORM:
        case TextureFormat::TxFmtR8_UINT:
        case TextureFormat::TxFmtR8_SINT:
        case TextureFormat::TxFmtR16_UNORM:
        case TextureFormat::TxFmtR16_SNORM:
        case TextureFormat::TxFmtR16_UINT:
        case TextureFormat::TxFmtR16_SINT:
        case TextureFormat::TxFmtR16_FLOAT:
        case TextureFormat::TxFmtR32_UINT:
        case TextureFormat::TxFmtR32_SINT:
        case TextureFormat::TxFmtR32_FLOAT:
            return GL_RED;

        case TextureFormat::TxFmtRG8_UNORM:
        case TextureFormat::TxFmtRG8_SNORM:
        case TextureFormat::TxFmtRG8_UINT:
        case TextureFormat::TxFmtRG8_SINT:
        case TextureFormat::TxFmtRG16_UNORM:
        case TextureFormat::TxFmtRG16_SNORM:
        case TextureFormat::TxFmtRG16_UINT:
        case TextureFormat::TxFmtRG16_SINT:
        case TextureFormat::TxFmtRG16_FLOAT:
        case TextureFormat::TxFmtRG32_UINT:
        case TextureFormat::TxFmtRG32_SINT:
        case TextureFormat::TxFmtRG32_FLOAT:
            return GL_RG;

        case TextureFormat::TxFmtRGB32_FLOAT:
        case TextureFormat::TxFmtRG11B10_FLOAT:
            return GL_RGB;

        case TextureFormat::TxFmtRGBA8_UNORM:
        case TextureFormat::TxFmtRGBA8_SNORM:
        case TextureFormat::TxFmtRGBA8_UINT:
        case TextureFormat::TxFmtRGBA8_SINT:
        case TextureFormat::TxFmtRGBA8_SRGB:
        case TextureFormat::TxFmtRGBA16_UNORM:
        case TextureFormat::TxFmtRGBA16_SNORM:
        case TextureFormat::TxFmtRGBA16_UINT:
        case TextureFormat::TxFmtRGBA16_SINT:
        case TextureFormat::TxFmtRGBA16_FLOAT:
        case TextureFormat::TxFmtRGBA32_FLOAT:
        case TextureFormat::TxFmtRGB10A2_UNORM:
            return GL_RGBA;

        case TextureFormat::TxFmtBGRA8_UNORM:
            return GL_BGRA;

        case TextureFormat::TxFmtD16_UNORM:
        case TextureFormat::TxFmtD32_FLOAT:
            return GL_DEPTH_COMPONENT;
        case TextureFormat::TxFmtD24_UNORM_S8_UINT:
        case TextureFormat::TxFmtD32_FLOAT_S8_UINT:
            return GL_DEPTH_STENCIL;

        default:
            return GL_NONE;
    }
}

GLenum TFmtGLType(TextureFormat fmt) {
    switch (fmt) {
        // -------- 8-bit --------
        case TextureFormat::TxFmtR8_UNORM:
        case TextureFormat::TxFmtRG8_UNORM:
        case TextureFormat::TxFmtRGBA8_UNORM:
        case TextureFormat::TxFmtBGRA8_UNORM:
        case TextureFormat::TxFmtRGBA8_SRGB:
            return GL_UNSIGNED_BYTE;
        case TextureFormat::TxFmtR8_SNORM:
        case TextureFormat::TxFmtRG8_SNORM:
        case TextureFormat::TxFmtRGBA8_SNORM:
            return GL_BYTE;
        case TextureFormat::TxFmtR8_UINT:
        case TextureFormat::TxFmtRG8_UINT:
        case TextureFormat::TxFmtRGBA8_UINT:
            return GL_UNSIGNED_BYTE;
        case TextureFormat::TxFmtR8_SINT:
        case TextureFormat::TxFmtRG8_SINT:
        case TextureFormat::TxFmtRGBA8_SINT:
            return GL_BYTE;

        // -------- 16-bit --------
        case TextureFormat::TxFmtR16_UNORM:
        case TextureFormat::TxFmtRG16_UNORM:
        case TextureFormat::TxFmtRGBA16_UNORM:
            return GL_UNSIGNED_SHORT;
        case TextureFormat::TxFmtR16_SNORM:
        case TextureFormat::TxFmtRG16_SNORM:
        case TextureFormat::TxFmtRGBA16_SNORM:
            return GL_SHORT;
        case TextureFormat::TxFmtR16_UINT:
        case TextureFormat::TxFmtRG16_UINT:
        case TextureFormat::TxFmtRGBA16_UINT:
            return GL_UNSIGNED_SHORT;
        case TextureFormat::TxFmtR16_SINT:
        case TextureFormat::TxFmtRG16_SINT:
        case TextureFormat::TxFmtRGBA16_SINT:
            return GL_SHORT;
        case TextureFormat::TxFmtR16_FLOAT:
        case TextureFormat::TxFmtRG16_FLOAT:
        case TextureFormat::TxFmtRGBA16_FLOAT:
            return GL_HALF_FLOAT;

        // -------- 32-bit --------
        case TextureFormat::TxFmtR32_UINT:
        case TextureFormat::TxFmtRG32_UINT:
            return GL_UNSIGNED_INT;
        case TextureFormat::TxFmtR32_SINT:
        case TextureFormat::TxFmtRG32_SINT:
            return GL_INT;
        case TextureFormat::TxFmtR32_FLOAT:
        case TextureFormat::TxFmtRG32_FLOAT:
        case TextureFormat::TxFmtRGB32_FLOAT:
        case TextureFormat::TxFmtRGBA32_FLOAT:
            return GL_FLOAT;

        // -------- Packed --------
        case TextureFormat::TxFmtRGB10A2_UNORM:
            return GL_UNSIGNED_INT_2_10_10_10_REV;
        case TextureFormat::TxFmtRG11B10_FLOAT:
            return GL_UNSIGNED_INT_10F_11F_11F_REV;

        // -------- Depth / Stencil --------
        case TextureFormat::TxFmtD16_UNORM:
            return GL_UNSIGNED_SHORT;
        case TextureFormat::TxFmtD24_UNORM_S8_UINT:
            return GL_UNSIGNED_INT_24_8;
        case TextureFormat::TxFmtD32_FLOAT:
            return GL_FLOAT;
        case TextureFormat::TxFmtD32_FLOAT_S8_UINT:
            return GL_FLOAT_32_UNSIGNED_INT_24_8_REV;

        // -------- Compressed --------
        case TextureFormat::TxFmtBC1_UNORM:
        case TextureFormat::TxFmtBC1_SRGB:
        case TextureFormat::TxFmtBC3_UNORM:
        case TextureFormat::TxFmtBC3_SRGB:
        case TextureFormat::TxFmtBC4_UNORM:
        case TextureFormat::TxFmtBC4_SNORM:
        case TextureFormat::TxFmtBC5_UNORM:
        case TextureFormat::TxFmtBC5_SNORM:
        case TextureFormat::TxFmtBC6H_UFLOAT:
        case TextureFormat::TxFmtBC6H_SFLOAT:
        case TextureFormat::TxFmtBC7_UNORM:
        case TextureFormat::TxFmtBC7_SRGB:
        case TextureFormat::TxFmtASTC_4x4_UNORM:
        case TextureFormat::TxFmtASTC_4x4_SRGB:
        case TextureFormat::TxFmtASTC_6x6_UNORM:
        case TextureFormat::TxFmtASTC_6x6_SRGB:
        case TextureFormat::TxFmtASTC_8x8_UNORM:
        case TextureFormat::TxFmtASTC_8x8_SRGB:
            return GL_NONE;
        default:
            return GL_NONE;
    }
}

GLenum TTypeGLTarget(TextureType type) {
    switch (type) {
        case TextureType::TxTTex2D:     return GL_TEXTURE_2D;
        case TextureType::TxTTex3D:     return GL_TEXTURE_3D;
        case TextureType::TxTCube:      return GL_TEXTURE_CUBE_MAP;
        case TextureType::TxTArray2D:   return GL_TEXTURE_2D_ARRAY;
        default: return GL_NONE;
    }
}

GLenum StgGLStage(ShaderStage stg) {
    switch (stg) {
        case ShaderStage::StgVertex:   return GL_VERTEX_SHADER;
        case ShaderStage::StgFragment: return GL_FRAGMENT_SHADER;
        case ShaderStage::StgCompute:  return GL_COMPUTE_SHADER;
        case ShaderStage::StgGeometry: return GL_GEOMETRY_SHADER;
        default: return 0;
    }
}

GLenum AttrTGLType(VertexAttributeType type) {
    switch (type) {
        case VertexAttributeType::VAtInt8:     return GL_BYTE;
        case VertexAttributeType::VAtUInt8:    return GL_UNSIGNED_BYTE;
        case VertexAttributeType::VAtInt16:    return GL_SHORT;
        case VertexAttributeType::VAtUInt16:   return GL_UNSIGNED_SHORT;
        case VertexAttributeType::VAtInt32:    return GL_INT;
        case VertexAttributeType::VAtUInt32:   return GL_UNSIGNED_INT;

        case VertexAttributeType::VAtFloat16:  return GL_HALF_FLOAT; // TODO: CRITICAL!! Verify extensions!!!
        case VertexAttributeType::VAtFloat32:  return GL_FLOAT;
        //case VertexAttributeType::VAtFloat64:  return GL_DOUBLE;

        default:
            assert(false && "Unsupported vertex attribute type");
            return GL_INVALID_ENUM;
    }
}

} // namespace axle::gfx
#endif
