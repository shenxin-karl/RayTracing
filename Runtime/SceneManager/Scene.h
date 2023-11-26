#pragma once
#include <memory>
#include <list>
#include "Foundation/NonCopyable.h"
#include "Object/InstanceID.hpp"

class GameObject;

class Scene : public NonCopyable {
	friend class SceneManager;
	using GameObjectList = std::list<std::shared_ptr<GameObject>>;
public:
	Scene();
	~Scene();
	void AddGameObject(std::shared_ptr<GameObject> pGameObject);
	void RemoveGameObject(std::shared_ptr<GameObject> pGameObject);
	void RemoveGameObject(InstanceID instanceId) {
		RemoveGameObjectInternal(instanceId);
	}
	void SetName(std::string name) {
		_name = std::move(name);
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
	void SetSceneID(int32_t sceneID);
	void RemoveGameObjectInternal(InstanceID instanceId);
private:
	// clang-format off
	std::string		_name;
	int32_t			_sceneID;
	GameObjectList	_gameObjects;
	// clang-format on
};
