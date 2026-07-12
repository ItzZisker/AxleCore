#include "axle/graphics/rendering/AX_RenderProcedureShader.hpp"

namespace axle::gfx
{

ThreadInvocation<utils::ExError> RPShaderManager::DescPush(const utils::UUID& id, const RPShaderDesc& desc) {
    return ThreadInvocation<utils::ExError>(m_Thread, [&, idCpy = id, descCpy = desc](){
        if (m_DescsById.find(idCpy) != m_DescsById.end()) {
            return utils::ExError("An instance of ShaderProcDesc with same UUID already exists!");
        }
        m_DescsById[idCpy] = descCpy;
        return utils::ExError::NoError();
    });
}

ThreadInvocation<utils::ExError> RPShaderManager::DescRemove(const utils::UUID& id) {
    return ThreadInvocation<utils::ExError>(m_Thread, [&, idCpy = id](){
        if (m_DescsById.find(idCpy) == m_DescsById.end()) {
            return utils::ExError("No instance of ShaderProcDesc with target UUID was found!");
        }
        m_DescsById.erase(idCpy);
        return utils::ExError::NoError();
    });
}

ThreadInvocation<utils::ExResult<RPShaderDesc>> RPShaderManager::DescGet(const utils::UUID& id) {
    return ThreadInvocation<utils::ExResult<RPShaderDesc>>(m_Thread, [&, idCpy = id]() {
        if (m_DescsById.find(idCpy) == m_DescsById.end()) {
            return utils::ExResult<RPShaderDesc>(
                utils::ExError("No instance of ShaderProcDesc with target UUID was found!")
            );
        }
        return utils::ExResult<RPShaderDesc>(m_DescsById[idCpy]);
    });
}

utils::ExResult<ShaderHandle> RPShaderManager::GetOrGenerateUnsafe(const utils::UUID& shaderId, const MeshVertexLayout& layout) {
    // TODO: this
}

ThreadInvocation<utils::ExResult<ShaderHandle>> RPShaderManager::GetOrGenerate(const utils::UUID& shaderId, const MeshVertexLayout& layout) {
    return ThreadInvocation<utils::ExResult<ShaderHandle>>(m_Thread, [&, shaderIdCpy = shaderId, layoutCpy = layout]() -> utils::ExResult<ShaderHandle> {
        return RPShaderManager::GetOrGenerateUnsafe(shaderIdCpy, layoutCpy);
    })
}

}