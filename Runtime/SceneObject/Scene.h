#pragma once
#include <memory>
#include <list>
#include "Foundation/NonCopyable.h"
#include "Object/InstanceID.hpp"
#include "SceneID.hpp"

class GameObject;
class SceneLightManager;

class Scene : private NonCopyable {
	friend class SceneManager;
	using GameObjectList = std::list<std::shared_ptr<GameObject>>;
public:
	Scene();
	~Scene();
public:
	void AddGameObject(std::shared_ptr<GameObject> pGameObject);
	void RemoveGameObject(const std::shared_ptr<GameObject> &pGameObject);
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
	// clang-format off
	std::string							_name;
	SceneID								_sceneID;
	GameObjectList						_gameObjects;
	std::unique_ptr<SceneLightManager>	_pLightManager;
	// clang-format on
};
