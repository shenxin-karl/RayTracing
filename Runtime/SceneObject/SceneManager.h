#pragma once
#include "SceneID.hpp"
#include "Foundation/Singleton.hpp"

class Scene;
class SceneManager : public Singleton<SceneManager>{
public:
	SceneManager();
	~SceneManager() override;
public:
	void OnCreate();
	void OnDestroy();
	auto CreateScene(std::string_view name) -> Scene *;
	auto GetScene(std::string_view name) const -> Scene *;
	auto GetOrCreateScene(std::string_view name) -> Scene *;
	auto GetScene(SceneID sceneID) const -> Scene *;
	void RemoveScene(std::string_view name);
private:
	std::vector<std::unique_ptr<Scene>> _scenes;
};
