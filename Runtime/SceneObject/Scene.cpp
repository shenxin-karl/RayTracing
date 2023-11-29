#include "Scene.h"
#include "Object/GameObject.h"
#include "Foundation/Exception.h"
#include "SceneLightManager.h"

Scene::Scene() {
    _pLightManager = std::make_unique<SceneLightManager>();
}

Scene::~Scene() {
}

void Scene::AddGameObject(std::shared_ptr<GameObject> pGameObject) {
    if (pGameObject->GetSceneID() == _sceneID) {
        return;
    }
    Exception::CondThrow(!pGameObject->GetSceneID().IsValid(), "This game object has been added to the scene");
    pGameObject->OnAddToScene(_sceneID);
    _gameObjects.push_back(std::move(pGameObject));
}

void Scene::RemoveGameObject(const std::shared_ptr<GameObject> &pGameObject) {
    RemoveGameObjectInternal(pGameObject->GetInstanceID());
}

void Scene::RemoveGameObjectInternal(InstanceID instanceId) {
    for (auto it = _gameObjects.begin(); it != _gameObjects.end(); ++it) {
        if ((*it)->GetInstanceID() == instanceId) {
            (*it)->OnRemoveFormScene();
            _gameObjects.erase(it);
            return;
        }
    }
}

void Scene::OnCreate(std::string name, SceneID sceneID) {
    _name = std::move(name);
    _sceneID = sceneID;
}

void Scene::OnDestroy() {
    while (!_gameObjects.empty()) {
        _gameObjects.back()->OnRemoveFormScene();
        _gameObjects.pop_back();
    }
}