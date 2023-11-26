#pragma once
#include <memory>
#include <vector>
#include "Foundation/NonCopyable.h"

class GameObject;

class Scene : public NonCopyable {
	using GameObjectContainer = std::vector<std::shared_ptr<GameObject>>;
public:
private:

	GameObjectContainer _gameObjects;
};
