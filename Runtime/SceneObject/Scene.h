#pragma once
#include <memory>
#include <list>
#include "Foundation/NonCopyable.h"
#include "Object/InstanceID.hpp"
#include "SceneID.hpp"
#include "Foundation/Memory/SharedPtr.hpp"
#include "Utils/GlobalCallbacks.h"

class GameObject;
class SceneLightManager;

class Scene : private NonCopyable {
	friend class SceneManager;
	using GameObjectList = std::list<SharedPtr<GameObject>>;
public:
	Scene();
	~Scene();
public:
	void AddGameObject(SharedPtr<GameObject> pGameObject);
	void RemoveGameObject(const SharedPtr<GameObject> &pGameObject);
	void RemoveGameObject(InstanceID instanceId) {
		RemoveGameObjectInternal(instanceId);
	}
	auto GetName() const -> const std::string & {
		return _name;
	}
	auto GetSceneID() const -> SceneID {
		return _sceneID;
	}
	auto GetGameObjectCount() const -> size_t {
		return _gameObjects.size();
	}
	auto begin() -> GameObjectList::iterator {
		return _gameObjects.begin();
	}
	auto begin() const -> GameObjectList::const_iterator {
		return _gameObjects.begin();
	}
	auto end() -> GameObjectList::iterator {
		return _gameObjects.end();
	}
	auto end() const -> GameObjectList::const_iterator {
		return _gameObjects.end();
	}
	auto GetSceneLightManager() const -> SceneLightManager * {
		return _pLightManager.get();
	}
private:
	void RemoveGameObjectInternal(InstanceID instanceId);
	void OnCreate(std::string name, SceneID sceneID);
	void OnDestroy();
private:
	void InvokeTickFunc(void (GameObject::*pTickFunc)());
	void OnPreUpdate(GameTimer &timer);
	void OnUpdate(GameTimer &timer);
	void OnPostUpdate(GameTimer &timer);
	void OnPreRender(GameTimer &timer);
	void OnRender(GameTimer &timer);
	void OnPostRender(GameTimer &timer);
private:
	// clang-format off
	std::string							_name;
	SceneID								_sceneID;
	GameObjectList						_gameObjects;
	std::unique_ptr<SceneLightManager>	_pLightManager;

	CallbackHandle						_preUpdateCallbackHandle;
	CallbackHandle						_updateCallbackHandle;
	CallbackHandle						_postUpdateCallbackHandle;
	CallbackHandle						_preRenderCallbackHandle;
	CallbackHandle						_renderCallbackHandle;
	CallbackHandle						_postRenderCallbackHandle;
	// clang-format on
};
