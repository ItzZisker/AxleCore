#include "axle/graphics/rendering/AX_RenderProcedureShader.hpp"
#include "axle/graphics/cmd/AX_PipelineManager.hpp"

#include "axle/utils/AX_Universal.hpp"

#include "eAX_EmbeddedShaders.hpp"

namespace axle::gfx
{

std::size_t RPShaderContext_Hash(const RPShaderContext& ctx) {
    std::size_t h{0};
    utils::HashEnum(h, ctx.transformInput);
    utils::HashCombine(h, std::hash<utils::UUID>{}(ctx.shaderId));
    utils::HashCombine(h, RPipeline_Hash_VertexLayout(ctx.vertexLayout));
    utils::HashCombine(h, ctx.skinned);
}

RPShaderManager::RPShaderManager(ThreadGfxScope gfxThread)
    : ThreadOwned(gfxThread) {}

RPShaderManager::~RPShaderManager() {
    
}

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

ShaderModuleDesc ByEmbedded(const eshdr::EmbeddedShader& es) {
    ShaderModuleDesc desc;
    desc.modName = es.moduleName;
    desc.codeBlob = utils::URaw(utils::URawView((uint8_t*)es.source.data(), es.source.size()));
    return desc;
}

utils::ExResult<gfx::ShaderHandle> RPShaderManager::GetOrGenerateUnsafe(const RPShaderContext& ctx) {
    auto rpCtxHash = RPShaderContext_Hash(ctx);

    if (m_ShaderCache.find(rpCtxHash) != m_ShaderCache.end())
        return m_ShaderCache[rpCtxHash];

    auto foundItr = m_DescsById.find(ctx.shaderId);
    if (foundItr == m_DescsById.end())
        return utils::ExError{"Shader descriptor with target UUID shaderId not found"};

    auto& rpShaderDesc = foundItr->second;

    std::vector<ShaderModuleDesc> moduleDescs;
    for (auto& rpShaderMod : rpShaderDesc.modules) moduleDescs.push_back(rpShaderMod);

    moduleDescs.push_back(ByEmbedded(*eshdr::Find("engine/transform")));

    switch (ctx.transformInput) {
        case RPShaderTransformInputType::GpuDriven:
            moduleDescs.push_back(ByEmbedded(*eshdr::Find("engine/transform_gpu_driven")));
            break;
        case RPShaderTransformInputType::Instanced:
            moduleDescs.push_back(ByEmbedded(*eshdr::Find("engine/transform_instanced")));
            break;
        case RPShaderTransformInputType::Uniform:
            moduleDescs.push_back(ByEmbedded(*eshdr::Find("engine/transform_uniform")));
            break;
    }

    ShaderDesc shaderDesc;
    shaderDesc.pipelineType = PipelineType::Graphics;
    shaderDesc.entryPointVertex = rpShaderDesc.entryPointVertex;
    shaderDesc.entryPointFragment = rpShaderDesc.entryPointFragment;
    shaderDesc.entryPointModuleIdx = rpShaderDesc.entryPointModuleIdx;
    shaderDesc.modules = {std::move(moduleDescs)};
    shaderDesc.defines = rpShaderDesc.defines;

    auto gbgfx = m_Thread->GetContext();

    gbgfx->CreateProgram()
}

ThreadInvocation<utils::ExResult<gfx::ShaderHandle>> RPShaderManager::GetOrGenerate(const RPShaderContext& ctx) {
    return MThreadInvocation(m_Thread, [&, ctxCpy = ctx](){
        return RPShaderManager::GetOrGenerateUnsafe(ctxCpy);
    });
}

}