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

// key : Mesh Vertex Layout Hash -> std::size_t
// value : MVL = Mesh Vertex Layout
using ShaderByMVL = std::unordered_map<std::size_t, gfx::ShaderHandle>;

class RPShaderManager : AX_THR_RENDER_OWNED {
private:
    std::unordered_map<utils::UUID, RPShaderDesc> m_DescsById;
    std::unordered_map<utils::UUID, ShaderByMVL> m_ShaderCache;

    std::vector<gfx::ShaderHandle> m_HeldHandles;

    utils::UUID m_Transaction{utils::UUID::Generate()};
public:
    ThreadInvocation<utils::ExError> DescPush(const utils::UUID& id, const RPShaderDesc& desc);
    ThreadInvocation<utils::ExError> DescRemove(const utils::UUID& id);
    ThreadInvocation<utils::ExResult<RPShaderDesc>> DescGet(const utils::UUID& id);

    virtual utils::ExResult<gfx::ShaderHandle> GetOrGenerateUnsafe(const utils::UUID& shaderId, const MeshVertexLayout& layout);

    ThreadInvocation<utils::ExResult<gfx::ShaderHandle>> GetOrGenerate(const utils::UUID& shaderId, const MeshVertexLayout& layout);

    utils::UUID GetTransaction() const { return m_Transaction; }
};

}
