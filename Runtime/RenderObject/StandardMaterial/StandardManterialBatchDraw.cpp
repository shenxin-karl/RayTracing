#include "StandardManterialBatchDraw.h"
#include "StandardMaterial.h"
#include "StandardMaterialDataManater.h"
#include "Components/Transform.h"
#include "RenderObject/RenderObject.h"

void StandardMaterialBatchDraw::Draw(std::span<RenderObject *const> batch, const GlobalShaderParam &globalShaderParam) {
    dx::GraphicsContext *pGfxCtx = globalShaderParam.pGfxCtx;

    // bind pipeline state object
    ID3D12PipelineState *pPipelineState = batch[0]->pMaterial->_pPipelineState;
    dx::RootSignature *pRootSignature = batch[0]->pMaterial->_pRootSignature;
    pGfxCtx->SetGraphicsRootSignature(pRootSignature);
    pGfxCtx->SetPipelineState(pPipelineState);

    pGfxCtx->SetGraphicsRootConstantBufferView(StandardMaterialDataManager::ePrePass, globalShaderParam.cbPrePassCBuffer);
    pGfxCtx->SetGraphicsRootConstantBufferView(StandardMaterialDataManager::eLighting, globalShaderParam.cbDirectionalLightBuffer);

    size_t index = 0;
    while (index < batch.size()) {
        // collection texture srv
        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> handles;
        std::unordered_map<D3D12_CPU_DESCRIPTOR_HANDLE, size_t> handleIndexMap;

        auto GetTextureIndex = [&](const dx::SRV &srv) {
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
                    handleIndexMap.emplace_hint(iter, std::make_pair(handle, handles.size()-1));
                }
            }
            ++batchIdx;
        }



        // texture list bindless 
        pGfxCtx->SetDynamicViews(StandardMaterialDataManager::eTextureList, handles);
        for (size_t i = index; i != batchIdx; ++i) {
	        Transform *pTransform = batch[i]->pTransform;
	        cbuffer::CbPreObject cbPreObject = cbuffer::MakePreObject(pTransform);
            pGfxCtx->SetComputeRootDynamicConstantBuffer(StandardMaterialDataManager::ePreObject, cbPreObject);

            StandardMaterial *pMaterial = batch[i]->pMaterial;
			pMaterial->_cbPreMaterial.albedoTexIndex = GetTextureIndex(pMaterial->_textureHandles[StandardMaterial::eAlbedoTex]);
			pMaterial->_cbPreMaterial.ambientOcclusionTexIndex = GetTextureIndex(pMaterial->_textureHandles[StandardMaterial::eAmbientOcclusionTex]);
			pMaterial->_cbPreMaterial.emissionTexIndex = GetTextureIndex(pMaterial->_textureHandles[StandardMaterial::eEmissionTex]);
			pMaterial->_cbPreMaterial.metalRoughnessTexIndex = GetTextureIndex(pMaterial->_textureHandles[StandardMaterial::eMetalRoughnessTex]);
			pMaterial->_cbPreMaterial.normalTexIndex = GetTextureIndex(pMaterial->_textureHandles[StandardMaterial::eNormalTex]);
            pGfxCtx->SetComputeRootDynamicConstantBuffer(StandardMaterialDataManager::eMaterial, pMaterial->_cbPreMaterial);


        }

        index = batchIdx;
    }
}
