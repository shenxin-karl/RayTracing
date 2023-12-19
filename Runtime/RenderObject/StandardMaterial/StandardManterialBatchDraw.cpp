#include "StandardManterialBatchDraw.h"
#include "StandardMaterial.h"
#include "RenderObject/RenderObject.h"

void StandardMaterialBatchDraw::Draw(std::span<RenderObject *const> batch, const GlobalShaderParam &globalShaderParam) {
    size_t index = 0;
    while (index < batch.size()) {
        std::unordered_map<D3D12_CPU_DESCRIPTOR_HANDLE, size_t> handleIndexMap;
        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> handles;
        size_t batchIdx = index;
        while (batchIdx < batch.size() && handles.size() < dx::kDynamicDescriptorMaxView) {
	        StandardMaterial *pMaterial = batch[batchIdx]->pMaterial;
            for (dx::SRV &handle : pMaterial->_textureHandles) {
	            
            }
            ++batchIdx;
        }


    }
}
