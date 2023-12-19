#include "ForwardPass.h"
#include "Foundation/Logger.h"
#include "RenderObject/RenderObject.h"
#include "RenderObject/StandardMaterial/StandardMaterial.h"

void ForwardPass::DrawBatchList(const std::vector<RenderObject *> &batchList,
    const GlobalShaderParam &globalShaderParam) {

    size_t index = 0;
    while (index < batchList.size()) {
        size_t materialID = batchList[index]->pMaterial->GetMaterialID();
        size_t pipelineID = batchList[index]->pMaterial->GetPipelineID();
        size_t first = index++;
        while (index < batchList.size()) {
            if (materialID == batchList[index]->pMaterial->GetMaterialID() &&
                pipelineID == batchList[index]->pMaterial->GetPipelineID()) {
	            ++index;
            } else {
	            break;
            }
        }

        if (materialID >= sMaterialBatchDrawItems.size()) {
	        Logger::Error("No BatchDraw object was found for MaterialID: {}", materialID);
            break;
        }

        size_t count = index - first;
        std::span batch = { batchList.begin() + first, count };
		sMaterialBatchDrawItems[materialID].pMaterialBatchDraw->Draw(batch, globalShaderParam);
    }
}

auto ForwardPass::RegisterMaterialBatchDraw(std::string_view materialTypeName,
	std::unique_ptr<IMaterialBatchDraw> pMaterialBatchDraw) -> uint16_t {
    MainThread::EnsureMainThread();
#if !MODE_RELEASE
    for (auto &item : sMaterialBatchDrawItems) {
	    if (item.materialTypeName == materialTypeName) {
		    Exception::Throw("Register material batch d again");
	    }
    }
#endif

    uint16_t materialID = sMaterialBatchDrawItems.size();
    Assert(pMaterialBatchDraw != nullptr);
    sMaterialBatchDrawItems.push_back({ materialTypeName, std::move(pMaterialBatchDraw) });
    return materialID;
}

std::vector<ForwardPass::MaterialBatchDrawItem> ForwardPass::sMaterialBatchDrawItems = {};
