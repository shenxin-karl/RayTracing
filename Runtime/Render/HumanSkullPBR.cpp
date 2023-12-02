#include "HumanSkullPBR.h"
#include "Components/Camera.h"
#include "Components/CameraColtroller.h"
#include "Components/Light.h"
#include "Object/GameObject.h"
#include "SceneObject/Scene.h"
#include "SceneObject/SceneManager.h"

HumanSkullPBR::HumanSkullPBR() {
}

HumanSkullPBR::~HumanSkullPBR() {
}

void HumanSkullPBR::OnCreate(uint32_t numBackBuffer, HWND hwnd) {
	Renderer::OnCreate(numBackBuffer, hwnd);
	InitScene();
}

void HumanSkullPBR::OnDestroy() {
	Renderer::OnDestroy();
	SceneManager::GetInstance()->RemoveScene(_pScene->GetName());
	_pScene = nullptr;
}

void HumanSkullPBR::OnPreRender(GameTimer &timer) {
	Renderer::OnPreRender(timer);
}

void HumanSkullPBR::OnRender(GameTimer &timer) {
	Renderer::OnRender(timer);
}

void HumanSkullPBR::OnResize(uint32_t width, uint32_t height) {
	Renderer::OnResize(width, height);
}

void HumanSkullPBR::InitScene() {
	_pScene = SceneManager::GetInstance()->CreateScene("Scene");
	SetupCamera();
	SetupLight();
}

void HumanSkullPBR::SetupCamera() {
	std::shared_ptr<GameObject> pGameObject = GameObject::Create();
	pGameObject->AddComponent<Camera>();
	pGameObject->AddComponent<CameraController>();
	Transform *pTransform = pGameObject->GetComponent<Transform>();
	pTransform->SetLocalPosition(glm::vec3(25, 10, 0));
	pTransform->LookAt(glm::vec3(0));
	_pScene->AddGameObject(pGameObject);
}

void HumanSkullPBR::SetupLight() {
	std::shared_ptr<GameObject> pGameObject = GameObject::Create();
	pGameObject->AddComponent<DirectionalLight>();

	Transform *pTransform = pGameObject->GetComponent<Transform>();
	glm::vec3 direction = glm::normalize(glm::vec3(0, -0.7, 0.5));
	glm::vec3 worldPosition(0, 1000, 0);
	glm::quat worldRotation = glm::Direction2Quaternion(direction);
	pTransform->SetWorldTRS(worldPosition, worldRotation, glm::vec3(1.f));
}

