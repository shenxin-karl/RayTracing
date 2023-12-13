#include "SceneLightManager.h"
#include "Foundation/Exception.h"

SceneLightManager::SceneLightManager() {
}

SceneLightManager::~SceneLightManager() {
}

auto SceneLightManager::GetDirectionalLightObjects() const -> const std::vector<GameObject *> & {
    return _directionalLightObjects;
}

void SceneLightManager::AddDirectionalLight(GameObject *pGameObject) {
    _directionalLightObjects.push_back(pGameObject);
}

void SceneLightManager::RemoveDirectionalLight(GameObject *pGameObject) {
    auto iter = std::ranges::find_if(_directionalLightObjects, [=](auto &item) { return item == pGameObject; });
    Assert(iter != _directionalLightObjects.end());
    _directionalLightObjects.erase(iter);
}
