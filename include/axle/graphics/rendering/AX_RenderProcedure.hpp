#pragma once

#include "axle/graphics/scene/AX_ModelInstance.hpp"

#include "axle/core/concurrency/AX_ThreadCycler.hpp"

#include "axle/utils/AX_UUID.hpp"

namespace axle::gfx
{

class RPShaderManager;
class PipelineManager;

struct RPPipelineResolverDesc {
    ThreadGfxScope gfxThread;

    SharedPtr<RPShaderManager> shaderProcMgr;
    SharedPtr<PipelineManager> pipelineMgr;
};

struct DrawCallContext;

struct RPPipelineResolveContext {
    RenderPassHandle renderPass;
    SharedPtr<scene::ModelInstance> modelInstance;
    const DrawCallContext& drawItem;
};

class RPPipelineResolver : AX_THR_RENDER_OWNED {
protected:
    RPPipelineResolverDesc m_Desc;

    std::unordered_map<std::size_t, RenderPipelineHandle> m_HoldingHandles;
public:
    RPPipelineResolver(const RPPipelineResolverDesc& desc);
    ~RPPipelineResolver(); // Delete Holding Handles by PipelineManager

    AX_NON_COPYABLE_NON_MOVABLE(RPPipelineResolver);

    // Get Or Create New one and put it in holding handles set
    virtual RenderPipelineHandle ResolveUnsafe(const RPPipelineResolveContext& drawCall);

    // We declared both, because ResolveUnsafe will NOT copy the drawcall arguments. by default (the invocation method)
    // we don't know about who is the thread caller, thus for safety, we have to make a copy of our drawcall being passed as argument
    // and will end up CPU-consuming for nothing, so we shall call ResolveUnsafe by the desired Rendering ThreadOwner (ThreadGfx)
    ThreadInvocation<RenderPipelineHandle> Resolve(const RPPipelineResolveContext& drawCall);
};

class RenderBatch;

struct RenderProcedureDesc {
    ThreadGfxScope gfxThread;

    SharedPtr<ICommandList> commandList{nullptr};
    SharedPtr<RenderBatch> renderBatch{nullptr};
    SharedPtr<RPPipelineResolver> pipelineResolver{nullptr};
};

class RenderProcedure : AX_THR_RENDER_OWNED {
private:
public:
    RenderProcedure(const RenderProcedureDesc& desc);
    ~RenderProcedure();

    AX_NON_COPYABLE_NON_MOVABLE(RenderProcedure);
protected:
    friend class RenderBatch;

    RenderProcedureDesc m_Desc;
    RenderPassHandle m_CurrentPass{utils::INVALID_HANDLE};

    void Execute();
};

}