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
class SceneRenderObjectManager;
class SceneRayTracingASManager;

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
	auto GetRenderObjectManager() const -> SceneRenderObjectManager * {
		return _pRenderObjectMgr.get();
	}
	auto GetRayTracingASManager() const -> SceneRayTracingASManager * {
		return _pRayTracingASMgr.get();
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
	using SceneRenderObjectManagerPtr = std::unique_ptr<SceneRenderObjectManager>;
	using SceneLightManagerPtr = std::unique_ptr<SceneLightManager>;
	using SceneRayTracingASManagerPtr = std::unique_ptr<SceneRayTracingASManager>;
	// clang-format off
	std::string					_name;
	SceneID						_sceneID;
	GameObjectList				_gameObjects;
	SceneLightManagerPtr		_pLightManager;
	SceneRenderObjectManagerPtr	_pRenderObjectMgr;
	SceneRayTracingASManagerPtr	_pRayTracingASMgr;

	CallbackHandle				_preUpdateCallbackHandle;
	CallbackHandle				_updateCallbackHandle;
	CallbackHandle				_postUpdateCallbackHandle;
	CallbackHandle				_preRenderCallbackHandle;
	CallbackHandle				_renderCallbackHandle;
	CallbackHandle				_postRenderCallbackHandle;
	// clang-format on
};
