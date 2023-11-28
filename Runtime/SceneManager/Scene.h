#pragma once
#include <memory>
#include <list>
#include "Foundation/NonCopyable.h"
#include "Object/InstanceID.hpp"

class GameObject;

class Scene : private NonCopyable {
	friend class SceneManager;
	using GameObjectList = std::list<std::shared_ptr<GameObject>>;
public:
	Scene() = default;
	~Scene() = default;
public:
	void AddGameObject(std::shared_ptr<GameObject> pGameObject);
	void RemoveGameObject(std::shared_ptr<GameObject> pGameObject);
	void RemoveGameObject(InstanceID instanceId) {
		RemoveGameObjectInternal(instanceId);
	}
	auto GetName() const -> const std::string & {
		return _name;
	}
	auto GetSceneID() const -> int32_t {
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
private:
	void RemoveGameObjectInternal(InstanceID instanceId);
	void OnCreate(std::string name, int32_t sceneID);
	void OnDestroy();
private:
	// clang-format off
	std::string		_name;
	int32_t			_sceneID;
	GameObjectList	_gameObjects;
	// clang-format on
};
