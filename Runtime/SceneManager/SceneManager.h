#pragma once
#include "Foundation/Singleton.hpp"

class Scene;
class SceneManager : public Singleton<SceneManager>{
public:
	SceneManager() = default;
	~SceneManager() override = default;
public:
	void OnCreate();
	void OnDestroy();
	auto CreateScene(std::string_view name) -> Scene *;
	auto GetScene(std::string_view name) const -> Scene *;
	auto GetOrCreateScene(std::string_view name) -> Scene *;
	auto GetScene(int32_t sceneID) const -> Scene *;
private:
	std::vector<std::unique_ptr<Scene>> _scenes;
};
