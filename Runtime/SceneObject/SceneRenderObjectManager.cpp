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
    key1 = (key1 | static_cast<uint64_t>(renderGroup) << 48);
}

auto RenderObjectKey::GetRenderGroup() const -> uint16_t {
    return (key1 & 0xFFFF000000000000) >> 48;
}

void RenderObjectKey::SetPriority(uint16_t priority) {
    key1 = (key1 | static_cast<uint64_t>(priority) << 32);
}

auto RenderObjectKey::GetPriority() const -> uint16_t {
    return (key1 & 0x0000FFFF0000000) >> 32;
}

void RenderObjectKey::SetPipelineID(uint32_t variantID) {
    key1 = (key1 | variantID);
}

auto RenderObjectKey::GetPipelineID() const -> uint32_t {
    return key1 & (0x00000000FFFFFFFF);
}

void RenderObjectKey::SetDepthSquare(double depthSqr) {
    uint64_t integerDepthSqr = std::bit_cast<uint64_t>(depthSqr);
    if (RenderGroup::IsTransparent(GetRenderGroup())) {
        integerDepthSqr = ~integerDepthSqr;
    }
    key2 = integerDepthSqr;
}

auto RenderObjectKey::GetDepthSquare() const -> double {
    uint64_t integerDepthSqr = key2;
    if (RenderGroup::IsTransparent(GetRenderGroup())) {
	    integerDepthSqr = ~integerDepthSqr;
    }
    return std::bit_cast<double>(integerDepthSqr);
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

    FetchRenderObject(_transparentRenderObjects, &RenderGroup::IsTransparent);
    FetchRenderObject(_alphaTestRenderObjects, &RenderGroup::IsAlphaTest);
    FetchRenderObject(_opaqueRenderObjects, &RenderGroup::IsOpaque);
}

std::strong_ordering operator<=>(const RenderObjectKey &lhs, const RenderObjectKey &rhs) {
    if (lhs.key1 > rhs.key1) {
        return std::strong_ordering::greater;
    }
    return lhs.key2 <=> rhs.key2;
}
