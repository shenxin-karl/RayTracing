#include "Scene.h"
#include "Object/GameObject.h"

Scene::Scene(): _sceneID(0) {
}

Scene::~Scene() {
}

void Scene::AddGameObject(std::shared_ptr<GameObject> pGameObject) {
	if (pGameObject->GetSceneID() == _sceneID) {
		return;
	}
	pGameObject->OnAddToScene();
	_gameObjects.push_back(std::move(pGameObject));
}

void Scene::RemoveGameObject(std::shared_ptr<GameObject> pGameObject) {
	RemoveGameObjectInternal(pGameObject->GetInstanceID());
}

void Scene::SetSceneID(int32_t sceneID) {
	_sceneID = sceneID;
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
