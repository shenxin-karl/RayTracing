#include "Scene.h"
#include "Object/GameObject.h"
#include "Foundation/Exception.h"
#include "SceneLightManager.h"
#include "SceneRenderObjectManager.h"

Scene::Scene() {
    _pLightManager = std::make_unique<SceneLightManager>();
    _pRenderObjectMgr = std::make_unique<SceneRenderObjectManager>();

    // register scene callbacks
    _preUpdateCallbackHandle = GlobalCallbacks::Get().onPreUpdate.Register(this, &Scene::OnPreUpdate);
	_updateCallbackHandle = GlobalCallbacks::Get().onUpdate.Register(this, &Scene::OnUpdate);
	_postUpdateCallbackHandle = GlobalCallbacks::Get().onPostUpdate.Register(this, &Scene::OnPostUpdate);
	_preRenderCallbackHandle = GlobalCallbacks::Get().onPreRender.Register(this, &Scene::OnPreRender);
	_renderCallbackHandle = GlobalCallbacks::Get().onRender.Register(this, &Scene::OnRender);
	_postRenderCallbackHandle = GlobalCallbacks::Get().onPostRender.Register(this, &Scene::OnPostRender);
}

Scene::~Scene() {
}

void Scene::AddGameObject(SharedPtr<GameObject> pGameObject) {
    if (pGameObject->GetSceneID() == _sceneID) {
        return;
    }
    Exception::CondThrow(!pGameObject->GetSceneID().IsValid(), "This game object has been added to the scene");
    pGameObject->OnAddToScene(_sceneID);
    _gameObjects.push_back(std::move(pGameObject));
}

void Scene::RemoveGameObject(const SharedPtr<GameObject> &pGameObject) {
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

void Scene::InvokeTickFunc(void(GameObject::*pTickFunc)()) {
    for (auto &pGameObject : _gameObjects) {
	    if (pGameObject->GetActive()) {
            (pGameObject.Get()->*pTickFunc)();
	    }
    }
}

void Scene::OnPreUpdate(GameTimer &timer) {
    InvokeTickFunc(&GameObject::InnerOnPreUpdate);
}

void Scene::OnUpdate(GameTimer &timer) {
    InvokeTickFunc(&GameObject::InnerOnUpdate);
}

void Scene::OnPostUpdate(GameTimer &timer) {
    InvokeTickFunc(&GameObject::InnerOnPostUpdate);
}

void Scene::OnPreRender(GameTimer &timer) {
    InvokeTickFunc(&GameObject::InnerOnPreRender);
}

void Scene::OnRender(GameTimer &timer) {
    InvokeTickFunc(&GameObject::InnerOnRender);
}

void Scene::OnPostRender(GameTimer &timer) {
    InvokeTickFunc(&GameObject::InnerOnPostRender);
    _pRenderObjectMgr->Reset();
}
