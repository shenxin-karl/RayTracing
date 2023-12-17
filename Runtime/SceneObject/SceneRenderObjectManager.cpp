#include <bit>
#include "SceneRenderObjectManager.h"
#include "RenderObject/RenderGroup.hpp"
#include "Components/Transform.h"
#include "RenderObject/RenderObject.h"
#include "RenderObject/StandardMaterial.h"

RenderObjectKey::RenderObjectKey() : key1(0), key2(0) {
}

void RenderObjectKey::SetMaterialID(uint16_t materialID) {
    key1 = (key1 | static_cast<uint64_t>(materialID) << 16);
}

auto RenderObjectKey::GetMaterialID() const -> uint16_t {
    return (key1 & (0x00000000FFFF0000)) >> 16;
}

void RenderObjectKey::SetPipelineID(uint16_t variantID) {
    key1 = (key1 | variantID);
}

auto RenderObjectKey::GetPipelineID() const -> uint16_t {
    return key1 & (0x000000000000FFFF);
}

void RenderObjectKey::SetRenderGroup(uint16_t renderGroup) {
    key1 = (key1 | static_cast<uint64_t>(renderGroup) << 48);
}

auto RenderObjectKey::GetRenderGroup() const -> uint16_t {
    return (key1 & 0xFFFF000000000000) >> 48;
}

void RenderObjectKey::SetDepthSquare(double depthSqr) {
    uint64_t integerDepthSqr = std::bit_cast<uint64_t>(depthSqr);
    if (RenderGroup::IsTransparent(GetRenderGroup())) {
        integerDepthSqr = ~integerDepthSqr;
    }
    key2 = integerDepthSqr;
}

void RenderObjectKey::SetPriority(uint16_t priority) {
    key1 = (key1 | static_cast<uint64_t>(priority) << 32);
}

auto RenderObjectKey::GetPriority() const -> uint16_t {
    return (key1 & 0x0000FFFF0000000) >> 32;
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
        item.key.SetPriority(0);
        item.key.SetMaterialID(pRenderObject->pMaterial->GetMaterialID());
        item.key.SetPipelineID(pRenderObject->pMaterial->GetPipelineID());

        glm::vec3 vector = worldCameraPos - pRenderObject->pTransform->GetWorldPosition();
        float depthSqr = dot(vector, vector);
        item.key.SetDepthSquare(depthSqr);

        item.pRenderObject = pRenderObject;
        items.push_back(item);
    }

    std::ranges::stable_sort(items, [](const Item &lhs, const Item &rhs) { return lhs.key < rhs.key; });

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
    if (lhs.key1 > rhs.key1) {
        return std::strong_ordering::greater;
    }
    return lhs.key2 <=> rhs.key2;
}
