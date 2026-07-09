#include "axle/graphics/rendering/AX_RenderBatch.hpp"
#include "axle/graphics/rendering/AX_RenderProcedure.hpp"

#include "axle/graphics/scene/AX_ModelInstance.hpp"

#include "axle/core/concurrency/AX_ThreadCycler.hpp"

#include <exception>

namespace axle::gfx
{

RenderBatch::RenderBatch(ThreadGfxScope gfxThread, const RenderBatchDesc& desc)
    : ThreadOwned(gfxThread), m_Desc(desc) {
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

void RenderBatch::TraverseNode(const NodeTraversalParams& params) {
    auto& modelInstance = params.modelInstance;
    auto& nodeInstance = params.nodeInstance;

    auto& results = params.results;

    auto& modelDesc = modelInstance.GetModelDesc();
    for (uint32_t meshId : nodeInstance.GetMeshIds()) {
        DrawItem drawItem;

        const auto& meshBinds = modelDesc.meshBinds;

        const auto& gpuMesh = meshBinds.draw.at(meshId);
        const auto& gpuMat = meshBinds.materials.at(meshId);

        drawItem.meshMode = gpuMesh.indexed ? MeshMode::Indexed : MeshMode::Vertices;
        drawItem.vertices = gpuMesh.vertices;
        drawItem.indices = gpuMesh.indices;
        drawItem.resources = gpuMat.resourcesHandle;
        drawItem.vertexCount = gpuMesh.vertexCount;
        drawItem.indexCount = gpuMesh.indexCount;
        drawItem.sortKey = m_Desc.userSortKeyAssigner({modelInstance, nodeInstance, meshId});

        results.push_back(drawItem);
    }

    for (auto& child : nodeInstance.GetChildren()) {
        TraverseNode({modelInstance, child, results});
    }
}

void RenderBatch::GenerateDrawCalls(SharedPtr<scene::ModelInstance> modelInstance, std::deque<DrawItem>& out_all) {
    std::deque<DrawItem> part{};
    TraverseNode({*modelInstance, *modelInstance->GetRootNode().SyncCall(), part});
    out_all.insert(
        out_all.end(),
        std::make_move_iterator(part.begin()),
        std::make_move_iterator(part.end())
    );
}

void RenderBatch::Record(const RenderProcedure& renderProc) {
    if (!m_Thread->ValidateThread()) return;

    auto commandList = renderProc.m_Desc.commandList;
    auto gbgfx = m_Thread->GetContext();

    RenderPipelineHandle currentPipeline{utils::INVALID_HANDLE};
    ResourceSetHandle currentResources{utils::INVALID_HANDLE};

    std::deque<DrawItem> items;

    for (auto& modelInstance : m_TrackingInstances) {
        GenerateDrawCalls(modelInstance, items);
    }

    for (auto& item : items) {
        if (currentPipeline != item.pipeline) {
            commandList->BindRenderPipeline({item.pipeline});
            currentPipeline = item.pipeline;
            currentResources = {UINT32_MAX, UINT32_MAX};
        }

        commandList->BindVertexBuffer({item.vertices});
        if (item.meshMode == MeshMode::Indexed) {
            commandList->BindIndexBuffer({item.vertices});
        }

        if (currentResources != item.resources) {
            commandList->BindResourceSet({item.resources});
            currentResources = item.resources;
        }

        if (item.meshMode == MeshMode::Indexed) {
            commandList->DrawIndexed({item.indexCount, item.firstIndex});
        } else {
            commandList->Draw({item.vertexCount, item.indexCount});
        }
    }
}

ThreadInvocationVoid RenderBatch::ClearInstances() {
    return ThreadInvocationVoid(m_Thread, [&](){
        m_TrackingInstances.clear();
        return VoidInvoke{};
    });
}

void RenderBatch::AddInstance0(SharedPtr<scene::ModelInstance> modelInstance) {
    m_TrackingInstances.insert(modelInstance);
}

void RenderBatch::RemoveInstance0(SharedPtr<scene::ModelInstance> modelInstance) {
    m_TrackingInstances.erase(modelInstance);
}

ThreadInvocationVoid RenderBatch::AddInstance(SharedPtr<scene::ModelInstance> modelInstance) {
    return ThreadInvocationVoid(m_Thread, [&](){
        AddInstance0(modelInstance);
        return VoidInvoke{};
    });
}

ThreadInvocationVoid RenderBatch::AddInstances(utils::Span<SharedPtr<scene::ModelInstance>> modelInstances) {
    return ThreadInvocationVoid(m_Thread, [&](){
        for (auto& modelInstance : modelInstances)
            AddInstance0(modelInstance);
        return VoidInvoke{};
    });
}

ThreadInvocationVoid RenderBatch::RemoveInstance(SharedPtr<scene::ModelInstance> modelInstance) {
    return ThreadInvocationVoid(m_Thread, [&](){
        RemoveInstance0(modelInstance);
        return VoidInvoke{};
    });
}

ThreadInvocationVoid RenderBatch::RemoveInstances(utils::Span<SharedPtr<scene::ModelInstance>> modelInstances) {
    return ThreadInvocationVoid(m_Thread, [&](){
        for (auto& modelInstance : modelInstances)
            RemoveInstance0(modelInstance);
        return VoidInvoke{};
    });
}

ThreadInvocationVoid RenderBatch::SetItemSort(const Predicate<DrawItem>& pred) {
    return ThreadInvocationVoid(m_Thread, [&](){
        m_Desc.itemsSort = pred;
        return VoidInvoke{};
    });
}

ThreadInvocationVoid RenderBatch::SetUserSortKeyAssigner(const UserSortKeyAssigner& assigner) {
    return ThreadInvocationVoid(m_Thread, [&](){
        m_Desc.userSortKeyAssigner = assigner;
        return VoidInvoke{};
    });
}

// ThreadInvocation<utils::ExResult<RLHandle>> RenderBatch::Register(SharedPtr<RenderLayer> rl, const RLStage& stage, uint32_t sortKey) {
//     return ThreadInvocation<utils::ExResult<RLHandle>>(m_Thread, [&](){
//         RLDesc desc;
//         desc.stage = stage;
//         desc.sortKey = sortKey;
//         desc.drawFunc = RenderBatch::Draw;
//         desc.updateFunc = nullptr;

//         auto rlhRes = rl->CreateLayer(desc).SyncCall();
//         if (!rlhRes.has_value()) {
//             return utils::ExResult<RLHandle>(rlhRes.error());
//         }
//         auto rlh = rlhRes.value();

//         m_Registries[rlh] = rl;
//         if (m_RegistriesRev.find(rl) == m_RegistriesRev.end()) {
//             m_RegistriesRev[rl] = {};
//         } else {
//             m_RegistriesRev[rl].push_back(rlh);
//         }
//         return rlhRes;
//     });
// }

// ThreadInvocation<utils::ExError> RenderBatch::UnRegister(const RLHandle& rlh) {
//     return ThreadInvocation<utils::ExError>(m_Thread, [&](){
//         if (m_Registries.find(rlh) == m_Registries.end()) {
//             return utils::ExError{"RenderLayer handle is not registered"};
//         }
//         {
//             auto rl = m_Registries[rlh];
//             rl->RemoveLayer(rlh);
//             m_RegistriesRev.erase(rl);
//         }
//         m_Registries.erase(rlh);
//         return utils::ExError::NoError();
//     });
// }

}