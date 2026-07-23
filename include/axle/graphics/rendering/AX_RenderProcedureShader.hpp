#pragma once

#include "axle/core/concurrency/AX_ThreadCycler.hpp"

#include "axle/graphics/AX_GraphicsParams.hpp"

#include "axle/utils/AX_UUID.hpp"

namespace axle::gfx
{

struct RPShaderDesc {
    utils::CowSpan<gfx::ShaderModuleDesc> modules;
    utils::CowSpan<gfx::ShaderSpecialization> specializations;

    utils::CowSpan<const char*> defines{};

    std::string_view entryPointVertex{""};
    std::string_view entryPointFragment{""};

    uint32_t entryPointModuleIdx{0};
};

enum class RPShaderTransformInputType {
    Uniform,
    Instanced,
    GpuDriven
};

struct RPShaderContext {
    const utils::UUID& shaderId;
    const MeshVertexLayout& vertexLayout;
    RPShaderTransformInputType transformInput;
    bool skinned;
};

struct RPShaderKeep {
    gfx::ShaderHandle handle;
    gfx::ShaderInputState inputs;
};

std::size_t RPShaderContext_Hash(const RPShaderContext& ctx);

class RPShaderManager : AX_THR_RENDER_OWNED {
private:
    std::unordered_map<utils::UUID, RPShaderDesc> m_DescsById;
    std::unordered_map<std::size_t, RPShaderKeep> m_ShaderCache; // Keyed by hashed RPShaderContext

    utils::UUID m_Transaction{utils::UUID::Generate()};
public:
    RPShaderManager(ThreadGfxScope gfxThread);
    ~RPShaderManager() = default;

    AX_NON_COPYABLE_NON_MOVABLE(RPShaderManager);

    ThreadInvocation<utils::ExError> DescPush(const utils::UUID& id, const RPShaderDesc& desc);
    ThreadInvocation<utils::ExError> DescRemove(const utils::UUID& id);
    ThreadInvocation<utils::ExResult<RPShaderDesc>> DescGet(const utils::UUID& id);

    virtual utils::ExResult<gfx::ShaderHandle> GetOrGenerateUnsafe(const RPShaderContext& ctx);

    ThreadInvocation<utils::ExResult<gfx::ShaderHandle>> GetOrGenerate(const RPShaderContext& ctx);

    utils::UUID GetTransaction() const { return m_Transaction; }
};

}
