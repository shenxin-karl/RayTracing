#include "Light.h"
#include "Object/GameObject.h"
#include "SceneObject/Scene.h"
#include "SceneObject/SceneLightManager.h"
#include "SceneObject/SceneManager.h"

Light::Light() : _color(1.f), _intensity(1.f) {
}

Light::~Light() {
}

void Light::SetColor(const glm::vec3 &color) {
	_color = color;
}

void Light::SetIntensity(float intensity) {
	_intensity = intensity;
}

auto Light::GetColor() const -> const glm::vec3 & {
	return _color;
}

auto Light::GetIntensity() const -> float {
	return _intensity;
}

void DirectionalLight::OnAddToScene() {
	Light::OnAddToScene();
	Scene *pScene = SceneManager::GetInstance()->GetScene(GetGameObject()->GetSceneID());
	SceneLightManager *pSceneLightManager = pScene->GetSceneLightManager();
	pSceneLightManager->AddDirectionalLight(GetGameObject());
}

void DirectionalLight::OnRemoveFormScene() {
	Light::OnRemoveFormScene();
	Scene *pScene = SceneManager::GetInstance()->GetScene(GetGameObject()->GetSceneID());
	SceneLightManager *pSceneLightManager = pScene->GetSceneLightManager();
	pSceneLightManager->RemoveDirectionalLight(GetGameObject());
}
