#pragma once

#include "axle/graphics/scene/AX_ModelInstance.hpp"

#include "axle/core/concurrency/AX_ThreadCycler.hpp"

#include "axle/utils/AX_UUID.hpp"

namespace axle::gfx
{

struct RenderPipelineResolverDesc {
    ShaderHandle fallbackShader;
};

class RenderPipelineResolver : AX_THR_RENDER_OWNED {
private:
    std::unordered_map<SharedPtr<scene::ModelInstance>, ShaderHandle> m_ShaderByModel;
public:
    RenderPipelineHandle Resolve(SharedPtr<scene::ModelInstance> md);
};

struct RenderProcedureDesc {
    SharedPtr<ICommandList> commandList{nullptr};
    
};

class RenderProcedure : AX_THR_RENDER_OWNED {
private:
public:
    RenderProcedure(ThreadGfxScope gfxThread, const RenderProcedureDesc& desc);
    ~RenderProcedure();
protected:
    friend class RenderBatch;

    RenderProcedureDesc m_Desc;

    void Execute();
};

}