#include "StandardManterialBatchDraw.h"
#include "StandardMaterial.h"
#include "StandardMaterialDataManater.h"
#include "Components/Transform.h"
#include "RenderObject/GPUMeshData.h"
#include "RenderObject/Mesh.h"
#include "RenderObject/RenderObject.h"

void StandardMaterialBatchDraw::Draw(std::span<RenderObject *const> batch, const GlobalShaderParam &globalShaderParam) {
    dx::GraphicsContext *pGfxCtx = globalShaderParam.pGfxCtx;

    // bind pipeline state object
    ID3D12PipelineState *pPipelineState = batch[0]->pMaterial->_pPipelineState;
    dx::RootSignature *pRootSignature = batch[0]->pMaterial->_pRootSignature;
    pGfxCtx->SetGraphicsRootSignature(pRootSignature);
    pGfxCtx->SetPipelineState(pPipelineState);

    pGfxCtx->SetGraphicsRootConstantBufferView(StandardMaterialDataManager::ePrePass,
        globalShaderParam.cbPrePassCBuffer);
    pGfxCtx->SetGraphicsRootConstantBufferView(StandardMaterialDataManager::eLighting, globalShaderParam.cbLightBuffer);

    size_t index = 0;
    while (index < batch.size()) {
        // collection texture srv
        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> handles;
        std::unordered_map<D3D12_CPU_DESCRIPTOR_HANDLE, size_t> handleIndexMap;

        auto GetTextureIndex = [&](const dx::SRV &srv) -> size_t {
            auto iter = handleIndexMap.find(srv.GetCpuHandle());
            if (iter != handleIndexMap.end()) {
                return iter->second;
            }
            return 0;
        };

        size_t batchIdx = index;
        while (batchIdx < batch.size() && handles.size() < dx::kDynamicDescriptorMaxView) {
            StandardMaterial *pMaterial = batch[batchIdx]->pMaterial;
            for (dx::SRV &srv : pMaterial->_textureHandles) {
                D3D12_CPU_DESCRIPTOR_HANDLE handle = srv.GetCpuHandle();
                auto iter = handleIndexMap.find(handle);
                if (iter == handleIndexMap.end()) {
                    handles.push_back(handle);
                    handleIndexMap.emplace_hint(iter, std::make_pair(handle, handles.size() - 1));
                }
            }
            ++batchIdx;
        }

        // texture list bindless
        pGfxCtx->SetDynamicViews(StandardMaterialDataManager::eTextureList, handles);
        for (size_t i = index; i != batchIdx; ++i) {
            Transform *pTransform = batch[i]->pTransform;
            cbuffer::CbPreObject cbPreObject = cbuffer::MakeCbPreObject(pTransform);
            pGfxCtx->SetComputeRootDynamicConstantBuffer(StandardMaterialDataManager::ePreObject, cbPreObject);

            using TextureType = StandardMaterial::TextureType;
            StandardMaterial *pMaterial = batch[i]->pMaterial;
            StandardMaterial::CbPreMaterial &cbMaterial = pMaterial->_cbPreMaterial;
            cbMaterial.albedoTexIndex = GetTextureIndex(pMaterial->_textureHandles[TextureType::eAlbedoTex]);
            cbMaterial.ambientOcclusionTexIndex = GetTextureIndex(
                pMaterial->_textureHandles[TextureType::eAmbientOcclusionTex]);
            cbMaterial.emissionTexIndex = GetTextureIndex(pMaterial->_textureHandles[TextureType::eEmissionTex]);
            cbMaterial.metalRoughnessTexIndex = GetTextureIndex(
                pMaterial->_textureHandles[TextureType::eMetalRoughnessTex]);
            cbMaterial.normalTexIndex = GetTextureIndex(pMaterial->_textureHandles[TextureType::eNormalTex]);
            pGfxCtx->SetComputeRootDynamicConstantBuffer(StandardMaterialDataManager::eMaterial, cbMaterial);

            Mesh *pMesh = batch[i]->pMesh;
            const GPUMeshData *pGpuMeshData = pMesh->GetGPUMeshData();
            pGfxCtx->SetVertexBuffers(0, pGpuMeshData->GetVertexBufferView());
            if (pMesh->GetIndexCount() > 0) {
                pGfxCtx->SetIndexBuffer(pGpuMeshData->GetIndexBufferView());
            }

            for (const SubMesh &subMesh : pMesh->GetSubMeshes()) {
                if (subMesh.indexCount > 0) {
                    pGfxCtx->DrawIndexedInstanced(subMesh.indexCount,
                        1,
                        subMesh.baseIndexLocation,
                        subMesh.baseVertexLocation,
                        0);
                } else {
                    pGfxCtx->DrawInstanced(subMesh.vertexCount, 1, subMesh.baseVertexLocation, 0);
                }
            }
        }

        index = batchIdx;
    }
}
