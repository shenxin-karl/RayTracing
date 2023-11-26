#pragma once
#include "Foundation/Singleton.hpp"

class Scene;
class SceneManager : public Singleton<SceneManager>{
public:
	SceneManager();
	~SceneManager() override;
	auto CreateScene(std::string_view name) -> Scene *;
	auto GetScene(std::string_view name) const -> Scene *;
	auto GetOrCreateScene(std::string_view name) -> Scene *;
	auto GetScene(int32_t sceneID) const -> Scene *;
private:
	std::vector<std::unique_ptr<Scene>> _scenes;
};
