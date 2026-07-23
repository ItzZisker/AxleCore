#include "axle/graphics/rendering/AX_RenderProcedure.hpp"
#include "axle/graphics/rendering/AX_RenderBatch.hpp"

#include "axle/graphics/cmd/AX_PipelineManager.hpp"

namespace axle::gfx
{

RPPipelineResolver::RPPipelineResolver(const RPPipelineResolverDesc& desc)
    : ThreadOwned(desc.gfxThread), m_Desc(desc) {}

RPPipelineResolver::~RPPipelineResolver() {
    ThreadInvocationVoid(m_Thread, [&]() {
        for (auto& [hash, handle] : m_HoldingHandles)
            m_Desc.pipelineMgr->Destroy(hash);
        return VoidInvoke{};
    }).SyncCall();
}

RenderPipelineHandle RPPipelineResolver::ResolveUnsafe(const RPPipelineResolveContext& resolveCtx) {
    auto pipelineMgr = m_Desc.pipelineMgr;
    
    const auto& drawCallCtx = resolveCtx.drawItem;
    
    auto modelInstance = resolveCtx.modelInstance;
    auto renderPass = resolveCtx.renderPass;

    if (!pipelineMgr->OnOwnerThread() || 
        !modelInstance->OnOwnerThread() ||
        !m_Desc.shaderProcMgr->OnOwnerThread())
        return RenderPipelineHandle{utils::INVALID_HANDLE};

    auto& meshStates = modelInstance->m_MeshStates;
    scene::MaterialStateMesh desiredMeshState{modelInstance->m_FallbackMeshState};

    if (meshStates.find(drawCallCtx.meshId) != meshStates.end()) {
        desiredMeshState = meshStates[drawCallCtx.meshId];
    }

    gfx::RenderPipelineDesc pipelineDesc;
    pipelineDesc.renderPass = renderPass;
    pipelineDesc.raster = desiredMeshState.raster;
    pipelineDesc.depth = desiredMeshState.depth;
    pipelineDesc.blend = desiredMeshState.blend;

    const auto& drawBindsMap = modelInstance->m_Desc.meshBinds.draw;
    if (drawBindsMap.find(drawCallCtx.meshId) == drawBindsMap.end())
        return RenderPipelineHandle{utils::INVALID_HANDLE};

    pipelineDesc.vertexLayout = drawBindsMap.at(drawCallCtx.meshId).layout;
    
    RPShaderContext rpshdrCtx {
        .shaderId = desiredMeshState.shaderId,
        .vertexLayout = pipelineDesc.vertexLayout,
        .transformInput = RPShaderTransformInputType::Uniform, // TODO: Read Renderbatch descriptor and check if its instanced rendering or not, etc.
        .skinned = false // TODO: We have to know when to use the animation/skinned shader, the mesh should also contains the skin data (boneIds/weights)
    };

    auto resShdr = m_Desc.shaderProcMgr->GetOrGenerateUnsafe(rpshdrCtx);
    if (!resShdr.has_value())
        return RenderPipelineHandle{utils::INVALID_HANDLE};

    pipelineDesc.shader = resShdr.value();

    auto pipelineRes = pipelineMgr->GetOrCreate(pipelineDesc).Immediate();
    if (!pipelineRes.has_value())
        return RenderPipelineHandle{utils::INVALID_HANDLE};

    return pipelineRes.value();
}

ThreadInvocation<RenderPipelineHandle> RPPipelineResolver::Resolve(const RPPipelineResolveContext& resolveCtx) {
    return ThreadInvocation<RenderPipelineHandle>(m_Thread, [&, resCpy = resolveCtx]() -> RenderPipelineHandle {
        return RPPipelineResolver::ResolveUnsafe(resCpy);
    });
}

}