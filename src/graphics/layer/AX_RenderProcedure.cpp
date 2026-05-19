#include "axle/graphics/layer/AX_RenderProcedure.hpp"

namespace axle::gfx
{

RenderProcedure::RenderProcedure(SharedPtr<ICommandList> cmdList)
    : m_RenderCmds(cmdList) {}

RenderProcedure::~RenderProcedure() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    for (auto& [rl, rlh_s] : m_RegistriesRev) {
        for (auto& rlh : rlh_s) {
            rl->RemoveLayer(rlh);
        }
    }
}

/*
    TODO:
    - We need three different RenderProcedures:
        1. Direct Drawing (SLOW BUT SIMPLE):
            NOTE: Uses shader variant (#ifdef DIRECT_DRAW #endif)
            - RenderItemBuilder Builds items based off each MeshInstance:
                - Define RenderItemType (whether its Indexed/Vertices)
                - Define Vertex/Index Buffer handles (whether RenderItemType is Indexed/Vertices)
                - PreCreate Pipelines: Based off hashing Pipeline Descriptor via Shader Reflection
                - Create/Use a BufferBindingHelper:
                    - Update/Get UBOs first: i=0: matrices, Materials's properties per mesh (underlying color, opacity, shininess factor, GGX factor, etc.) in singular block.
                    - Update/Get Materials Textures: Starting from i=1: Resource bindings referencing Textures
                    If supported/configured-in-renderprocedure-desc for separate Samplers from Textures:
                        - Define "SEPARATE_SAMPLERS" in shaders, so we use the correct variant
                        - Update/Get Samplers: Configurable (Set by user in RenderProcedureDesc)
                    - Pack OR Get-from-cache all UBOs, Materials, and finally Create/Update The (ResourceSetHandle) and pass to RenderItem
            - For each RenderItem:
                - Unique-Bind Pipeline
                - Unique-Bind Vertex/Index Buffers
                - Unique-Bind ResourceSets (Materials, Textures, UBOs, etc.)
                - Issue Direct Draw/DrawIndexed Calls (whether RenderItemType is Indexed/Vertices)
                - Submit Commands
        2. Instanced Drawing (FASTER, "used for MeshInstances" BUT MORE COMPLEX)
            NOTE: Uses a different shader variant (#ifdef INSTANCED_DRAW #endif)
            - RenderItemBuilder Builds items based off each Mesh:
                - Define RenderItemType (whether its Indexed/Vertices)
                - Define Vertex/Index Buffer handles (whether RenderItemType is Indexed/Vertices)
                - PreCreate Pipelines: Based off hashing Pipeline Descriptor via Shader Reflection
                - Create/Use a BufferBindingHelper:
                    - Update/Get UBOs first: i=0: array of (matrices, Materials's properties per mesh (underlying color, opacity, shininess factor, GGX factor, etc.)) that each element
                      Index refers to the gl_DrawInstance (GL-Style) or SV_DrawInstance (DX-Style).
                    - Update/Get Materials Textures: Starting from i=1: Resource bindings referencing Textures
                    If supported/configured-in-renderprocedure-desc for separate Samplers from Textures:
                        - Define "SEPARATE_SAMPLERS" in shaders, so we use the correct variant
                        - Update/Get Samplers: Configurable (Set by user in RenderProcedureDesc)
                    - Pack OR Get-from-cache all UBOs, Materials, and finally Create/Update The (ResourceSetHandle) and pass to RenderItem
            - For each RenderItem:
                - Unique-Bind Pipeline
                - Unique-Bind Vertex/Index Buffers
                - Unique-Bind ResourceSets (Materials, Textures, UBOs, etc.)
                - Issue Direct Instanced Draw/DrawIndexed Calls (whether RenderItemType is Indexed/Vertices)
                - Submit Commands
        3. Indirect Instanced Drawing (QUICK, "used for MeshInstances" BUT MORE COMPLEX AND INDIRECT)
            NOTE: Uses the same variant as above (Instanced Drawing) basically "INSTANCED_DRAW" predefined in the shader same as above
            - RenderItemBuilder Builds items based off globally:
                - Define RenderItemType (whether its Indexed/Vertices)
                - Create/Use an IndirectBindingHelper
                - Reset the IndirectBindingHelper
                - For each Mesh (predefined previously)
                    - Define Vertex/Index Buffer handles (whether RenderItemType is Indexed/Vertices)
                    - PreCreate Pipelines: Based off hashing Pipeline Descriptor via Shader Reflection
                    - Create/Use a BufferBindingHelper:
                        - Update/Get UBOs first: i=0: array of (matrices, Materials's properties per mesh (underlying color, opacity, shininess factor, GGX factor, etc.)) that each element
                        Index refers to the gl_DrawInstance (GL-Style) or SV_DrawInstance (DX-Style).
                        - Update/Get Materials Textures: Starting from i=1: Resource bindings referencing Textures
                        If supported/configured-in-renderprocedure-desc for separate Samplers from Textures:
                            - Define "SEPARATE_SAMPLERS" in shaders, so we use the correct variant
                            - Update/Get Samplers: Configurable (Set by user in RenderProcedureDesc)
                        - Pack OR Get-from-cache all UBOs, Materials, and finally Create/Update The (ResourceSetHandle) and pass to RenderItem
                - Ensure Meshes are grouped by the same binding (Pipeline, Vertex/Index, ResourceSets, etc.)
                - For each Mesh group:
                    - Use the Index/Vertex Buffer alignments, strides, and parameters to Append Draw Commands onto IndirectBindingHelper
                - IndirectBindingHelper#Build() -> Modifies/Creates & returns an IBO which contains Draw Commands and pass it onto RenderItem        
            - For each RenderItem:
                - Unique-Bind Pipeline
                - Unique-Bind Vertex/Index Buffers
                - Unique-Bind Indirect Rendering Buffer
                - Unique-Bind ResourceSets (Materials, Textures, UBOs, etc.)
                - Issue Indirect Instanced Draw/DrawIndexed Calls (whether RenderItemType is Indexed/Vertices)
                - Submit Commands and let GPU draw based off IBO Draw Parameters.
*/
void RenderProcedure::Draw(SharedPtr<core::ThreadContextGfx> thrCtx, float dT, void* userPtr) {
    if (!thrCtx->ValidateThread()) return;

    auto& rp = *((RenderProcedure*)userPtr);
    std::lock_guard<std::mutex> lock(rp.m_Mutex);

    auto gbgfx = thrCtx->GetContext();

    RenderPipelineHandle currentPipeline{UINT32_MAX, UINT32_MAX};

    for (auto& item : rp.m_Items) {
        if (currentPipeline.IsSameHandles(item.pipeline)) {
            rp.m_RenderCmds->BindRenderPipeline({item.pipeline});
            currentPipeline = item.pipeline;
        }

        rp.m_RenderCmds->BindVertexBuffer({item.vertices});
        if (item.type == RenderItemType::Indexed) {
            rp.m_RenderCmds->BindIndexBuffer({item.vertices});
        }

        for (auto& res : item.resourcesPerInstance) {
            rp.m_RenderCmds->BindResourceSet({res});

            if (item.type == RenderItemType::Indexed) {
                rp.m_RenderCmds->DrawIndexed({item.indexCount, item.firstIndex});
            } else {
                rp.m_RenderCmds->Draw({item.vertexCount, item.indexCount});
            }
        }
    }
    rp.m_RenderCmds->End();
    rp.m_RenderCmds->Submit(thrCtx);
}

void RenderProcedure::ResetItems() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Items.clear();
}

void RenderProcedure::Submit(const RenderItem& item) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Items.push_back(item);
}

void RenderProcedure::Submit(utils::Span<RenderItem> itemsView) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    for (auto& item : itemsView) {
        m_Items.push_back(item);
    }
}

void RenderProcedure::ResetAndSubmit(const RenderItem& item) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Items.clear();
    m_Items.push_back(item);
}

void RenderProcedure::ResetAndSubmit(utils::Span<RenderItem> itemsView) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Items.clear();
    for (auto& item : itemsView) {
        m_Items.push_back(item);
    }
}

utils::ExResult<RLHandle> RenderProcedure::Register(SharedPtr<RenderLayer> rl, const RLStage& stage, uint32_t sortKey) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    RLDesc desc;
    desc.stage = stage;
    desc.sortKey = sortKey;
    desc.drawFunc = RenderProcedure::Draw;
    desc.updateFunc = nullptr;
    AX_DECL_OR_PROPAGATE(rlh, rl->CreateLayer(desc));

    m_Registries[rlh] = rl;
    if (m_RegistriesRev.find(rl) == m_RegistriesRev.end()) {
        m_RegistriesRev[rl] = {};
    } else {
        m_RegistriesRev[rl].push_back(rlh);
    }
    return rlh;
}

utils::ExError RenderProcedure::UnRegister(const RLHandle& rlh) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_Registries.find(rlh) == m_Registries.end()) {
        return {"RenderLayer handle is not registered"};
    }
    {
        auto rl = m_Registries[rlh];
        rl->RemoveLayer(rlh);
        m_RegistriesRev.erase(rl);
    }
    m_Registries.erase(rlh);
}

void RenderProcedure::Sort(const Predicate<RenderItem>& pred = RPROC_SORT_BY_WEIGHT) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    std::sort(m_Items.begin(), m_Items.end(), pred);
}

}