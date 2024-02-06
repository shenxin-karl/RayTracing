#include <bit>
#include "SceneRenderObjectManager.h"
#include "RenderObject/RenderGroup.hpp"
#include "Components/Transform.h"
#include "Foundation/Logger.h"
#include "RenderObject/RenderObject.h"
#include "RenderObject/Material.h"

RenderObjectKey::RenderObjectKey() : key1(0), key2(0) {
}

void RenderObjectKey::SetRenderGroup(uint16_t renderGroup) {
    this->renderGroup = renderGroup;
}

auto RenderObjectKey::GetRenderGroup() const -> uint16_t {
    return renderGroup;
}

void RenderObjectKey::SetPriority(uint16_t priority) {
    this->priority = priority;
}

auto RenderObjectKey::GetPriority() const -> uint16_t {
    return priority;;
}

void RenderObjectKey::SetPipelineID(uint32_t variantID) {
    this->pipelineID = variantID;
}

auto RenderObjectKey::GetPipelineID() const -> uint32_t {
    return pipelineID;
}

void RenderObjectKey::SetDepthSquare(double depthSqr) {
	this->distanceSqr = depthSqr;
}

auto RenderObjectKey::GetDepthSquare() const -> double {
	return distanceSqr;
}

void SceneRenderObjectManager::ClassifyRenderObjects(const glm::vec3 &worldCameraPos) {
    struct Item {
        RenderObjectKey key;
        RenderObject *pRenderObject;
    };

    std::vector<Item> items;
    items.reserve(_renderObjects.size());
    for (size_t i = 0; i < _renderObjects.size(); ++i) {
        RenderObject *pRenderObject = _renderObjects[i];
        Item item;
        item.key.SetRenderGroup(pRenderObject->pMaterial->GetRenderGroup());
        item.key.SetPriority(pRenderObject->priority);
        item.key.SetPipelineID(pRenderObject->pMaterial->GetPipelineID());

        glm::vec3 vector = worldCameraPos - pRenderObject->pTransform->GetWorldPosition();
        float depthSqr = dot(vector, vector);
        item.key.SetDepthSquare(depthSqr);

        item.pRenderObject = pRenderObject;
        items.push_back(item);
    }

    std::ranges::stable_sort(items, [](const Item &lhs, const Item &rhs) { return lhs.key < rhs.key; });

    static bool debugPrint = false;
    if (debugPrint) {
        for (auto item : items) {
	        Logger::Debug("RenderGroup: {}, Priority: {}, PipelineID: {}, DepthSqr: {}", 
                item.key.GetRenderGroup(),
                item.key.GetPriority(),
                item.key.GetPipelineID(),
                item.key.GetDepthSquare()
            );
        }
    }

    _opaqueRenderObjects.clear();
    _alphaTestRenderObjects.clear();
    _transparentRenderObjects.clear();

    auto it = items.begin();
    auto FetchRenderObject = [&](std::vector<RenderObject *> &result, bool (*pItemInRenderGroup)(uint16_t)) {
        while (it != items.end()) {
            if (pItemInRenderGroup(it->key.GetRenderGroup())) {
                result.push_back(it->pRenderObject);
                ++it;
            } else {
                break;
            }
        }
    };

    FetchRenderObject(_opaqueRenderObjects, &RenderGroup::IsOpaque);
    FetchRenderObject(_alphaTestRenderObjects, &RenderGroup::IsAlphaTest);
    FetchRenderObject(_transparentRenderObjects, &RenderGroup::IsTransparent);
}

std::strong_ordering operator<=>(const RenderObjectKey &lhs, const RenderObjectKey &rhs) {
    if (std::strong_ordering cmp = lhs.key1 <=> rhs.key1; cmp != std::strong_ordering::equal) {
        return cmp;
    }
    return lhs.key2 <=> rhs.key2;
}
