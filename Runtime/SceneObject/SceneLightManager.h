#pragma once
#include <vector>

class GameObject;

class SceneLightManager {
public:
    SceneLightManager();
    ~SceneLightManager();
public:
    auto GetDirectionalLightObjects() const -> const std::vector<GameObject *> &;
private:
    friend class DirectionalLight;
    void AddDirectionalLight(GameObject *pGameObject);
    void RemoveDirectionalLight(GameObject *pGameObject);
private:
    std::vector<GameObject *> _directionalLightObjects;
};
