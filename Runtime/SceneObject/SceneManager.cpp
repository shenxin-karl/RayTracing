#include "SceneManager.h"
#include "Scene.h"

SceneManager::SceneManager() {
}

SceneManager::~SceneManager() {
}

void SceneManager::OnCreate() {
}

void SceneManager::OnDestroy() {
	while (!_scenes.empty()) {
		_scenes.back()->OnDestroy();
		_scenes.pop_back();
	}
}

auto SceneManager::CreateScene(std::string_view name) -> Scene * {
	Assert(GetScene(name) == nullptr);
	_scenes.push_back(std::make_unique<Scene>());
	_scenes.back()->OnCreate(name.data(), SceneID(_scenes.size()));
	return _scenes.back().get();
}

auto SceneManager::GetScene(std::string_view name) const -> Scene * {
	for (const std::unique_ptr<Scene> &pScene : _scenes) {
		if (pScene->GetName() == name) {
			return pScene.get();
		}
	}
	return nullptr;
}

auto SceneManager::GetOrCreateScene(std::string_view name) -> Scene * {
	Scene *pScene = GetScene(name);
	if (pScene != nullptr) {
		return pScene;
	}
	return CreateScene(name);
}

auto SceneManager::GetScene(SceneID sceneID) const -> Scene * {
	for (const std::unique_ptr<Scene> &pScene : _scenes) {
		if (pScene->GetSceneID() == sceneID) {
			return pScene.get();
		}
	}
	return nullptr;
}
