#include "RenderPass.h"
#include "RenderObject/RenderObject.h"
#include "RenderObject/StandardMaterial.h"

void RenderPass::DrawBatchList(const std::vector<RenderObject *> &batchList, const DrawBatchListCallback &callback) {
    size_t index = 0;
    while (index < batchList.size()) {
        size_t pipelineID = batchList[index]->pMaterial->GetPipelineID();
        size_t first = index++;
        while (index < batchList.size()) {
            if (pipelineID == batchList[index]->pMaterial->GetPipelineID()) {
                ++index;
            } else {
                break;
            }
        }

        size_t count = index - first;
        std::span batch = {batchList.begin() + first, count};
        callback(batch);
    }
}
