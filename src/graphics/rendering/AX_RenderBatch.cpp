#include "axle/graphics/rendering/AX_RenderBatch.hpp"

#include "axle/graphics/scene/AX_ModelInstance.hpp"

#include "axle/core/concurrency/AX_ThreadCycler.hpp"

#include <exception>

#define ACT_EXERR_BATCH_PRED(statement, errorHandler)   \
auto __err = (statement);                               \
if (__err.IsValid())                                    \
    if (errorHandler(__err)) return;                    \

namespace axle::gfx
{

RenderBatch::RenderBatch(const RenderBatchDesc& desc)
    : m_Desc(desc) {
    if (!desc.gfxThread)
        throw std::runtime_error("RenderBatch: ThreadContextGfx is NULL");
    if (!desc.commandList)
        throw std::runtime_error("RenderBatch: CommandList is NULL");
}

RenderBatch::~RenderBatch() {
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
void RenderBatch::Draw(SharedPtr<core::ThreadContextGfx> thrCtx, float dT, void* userPtr) {
    if (!thrCtx->ValidateThread()) return;

    auto& rp = *((RenderBatch*)userPtr);
    std::lock_guard<std::mutex> lock(rp.m_Mutex);

    auto gbgfx = thrCtx->GetContext();

    RenderPipelineHandle currentPipeline{UINT32_MAX, UINT32_MAX};
    ResourceSetHandle currentResources{UINT32_MAX, UINT32_MAX};

    rp.m_Desc.commandList->Begin();

    for (auto& item : rp.m_Items) {
        if (currentPipeline != item.pipeline) {
            ACT_EXERR_BATCH_PRED(rp.m_Desc.commandList->BindRenderPipeline({item.pipeline}), rp.m_Desc.frameErrorHandler);
            currentPipeline = item.pipeline;
            currentResources = {UINT32_MAX, UINT32_MAX};
        }

        rp.m_Desc.commandList->BindVertexBuffer({item.vertices});
        if (item.meshMode == MeshMode::Indexed) {
            ACT_EXERR_BATCH_PRED(rp.m_Desc.commandList->BindIndexBuffer({item.vertices}), rp.m_Desc.frameErrorHandler);
        }

        if (currentResources != item.resources) {
            ACT_EXERR_BATCH_PRED(rp.m_Desc.commandList->BindResourceSet({item.resources}), rp.m_Desc.frameErrorHandler);
            currentResources = item.resources;
        }

        if (item.meshMode == MeshMode::Indexed) {
            ACT_EXERR_BATCH_PRED(rp.m_Desc.commandList->DrawIndexed({item.indexCount, item.firstIndex}), rp.m_Desc.frameErrorHandler);
        } else {
            ACT_EXERR_BATCH_PRED(rp.m_Desc.commandList->Draw({item.vertexCount, item.indexCount}), rp.m_Desc.frameErrorHandler);
        }
    }

    ACT_EXERR_BATCH_PRED(rp.m_Desc.commandList->End(), rp.m_Desc.frameErrorHandler);
    ACT_EXERR_BATCH_PRED(gbgfx->Execute(*rp.m_Desc.commandList), rp.m_Desc.frameErrorHandler);
}

void RenderBatch::RebuildDrawItems() {
    std::lock_guard<std::mutex> lock(m_Mutex);

}

void RenderBatch::Clear() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Items.clear();
    m_TrackingInstances.clear();
}

void RenderBatch::TraverseNode(
    scene::ModelInstance& modelInstance,
    scene::NodeInstance& nodeInstance,
    std::unordered_set<DrawItem>& results
) {
    auto& modelDesc = modelInstance.GetModelDesc();
    for (uint32_t meshId : nodeInstance.GetMeshIds()) {
        DrawItem drawItem;

        auto& gpuMesh = modelDesc.meshBounds.draw[meshId];
        auto& gpuMat = modelDesc.meshBounds.materials[meshId];
        
        drawItem.meshMode = gpuMesh.indexed ? MeshMode::Indexed : MeshMode::Vertices;
        drawItem.pipeline = modelDesc.pipelineBounds.pipelineHandle;
        drawItem.vertices = gpuMesh.vertices;
        drawItem.indices = gpuMesh.indices;
        drawItem.resources = gpuMat.resourcesHandle;
        drawItem.vertexCount = gpuMesh.vertexCount;
        drawItem.indexCount = gpuMesh.indexCount;
        drawItem.sortKey = m_Desc.userSortKeyAssigner({modelInstance, nodeInstance, meshId});

        results.insert(drawItem);
    }
    for (auto& child : nodeInstance.GetChildren()) {
        TraverseNode(modelInstance, child, results);
    }
}

void RenderBatch::AddInstance0(SharedPtr<scene::ModelInstance> modelInstance) {
    m_TrackingInstances.insert(modelInstance);
    std::unordered_set<DrawItem> drawItems{};
    TraverseNode(*modelInstance, modelInstance->GetRoot(), drawItems);
    m_Items.insert(
        m_Items.end(),
        std::make_move_iterator(drawItems.begin()),
        std::make_move_iterator(drawItems.end())
    );
}

void RenderBatch::AddInstance(SharedPtr<scene::ModelInstance> modelInstance) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    AddInstance0(modelInstance);
    if (m_Desc.autoSort) RenderBatch::SortItems();
}

void RenderBatch::AddInstances(utils::Span<SharedPtr<scene::ModelInstance>> modelInstances) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    for (auto& modelInstance : modelInstances) 
        AddInstance0(modelInstance);
    if (m_Desc.autoSort) RenderBatch::SortItems();
}

void RenderBatch::SetItemSort(const Predicate<DrawItem>& pred) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Desc.itemsSort = pred;
}

void RenderBatch::SetUserSortKeyAssigner(const UserSortKeyAssigner& assigner) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Desc.userSortKeyAssigner = assigner;
}

utils::ExResult<RLHandle> RenderBatch::Register(SharedPtr<RenderLayer> rl, const RLStage& stage, uint32_t sortKey) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    RLDesc desc;
    desc.stage = stage;
    desc.sortKey = sortKey;
    desc.drawFunc = RenderBatch::Draw;
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

utils::ExError RenderBatch::UnRegister(const RLHandle& rlh) {
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

void RenderBatch::SortItems() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    std::sort(m_Items.begin(), m_Items.end(), m_Desc.itemsSort);
}

}