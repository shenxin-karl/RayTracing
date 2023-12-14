#include "GLTFSample.h"
#include "Components/Camera.h"
#include "Components/CameraColtroller.h"
#include "Components/Light.h"
#include "Components/Transform.h"
#include "Object/GameObject.h"
#include "SceneObject/GLTFLoader.h"
#include "SceneObject/Scene.h"
#include "SceneObject/SceneManager.h"
#include "Utils/AssetProjectSetting.h"

GLTFSample::GLTFSample() {
}

GLTFSample::~GLTFSample() {
}

void GLTFSample::OnCreate() {
	Renderer::OnCreate();
	InitScene();
}

void GLTFSample::OnDestroy() {
	Renderer::OnDestroy();
	SceneManager::GetInstance()->RemoveScene(_pScene->GetName());
	_pScene = nullptr;
}

void GLTFSample::OnPreRender(GameTimer &timer) {
	Renderer::OnPreRender(timer);
}

void GLTFSample::OnRender(GameTimer &timer) {
	Renderer::OnRender(timer);
}

void GLTFSample::OnResize(uint32_t width, uint32_t height) {
	Renderer::OnResize(width, height);
}

void GLTFSample::InitScene() {
	_pScene = SceneManager::GetInstance()->CreateScene("Scene");
	SetupCamera();
	SetupLight();
	LoadGLTF();
}

void GLTFSample::SetupCamera() {
	SharedPtr<GameObject> pGameObject = GameObject::Create();
	pGameObject->AddComponent<Camera>();
	pGameObject->AddComponent<CameraController>();
	Transform *pTransform = pGameObject->GetTransform();
	pTransform->SetLocalPosition(glm::vec3(25, 10, 0));
	pTransform->LookAt(glm::vec3(0));
	_pScene->AddGameObject(pGameObject);
}

void GLTFSample::SetupLight() {
	SharedPtr<GameObject> pGameObject = GameObject::Create();
	pGameObject->AddComponent<DirectionalLight>();

	Transform *pTransform = pGameObject->GetTransform();
	glm::vec3 direction = glm::normalize(glm::vec3(0, -0.7, 0.5));
	glm::vec3 worldPosition(0, 1000, 0);
	glm::quat worldRotation = glm::Direction2Quaternion(direction);
	pTransform->SetWorldTRS(worldPosition, worldRotation, glm::vec3(1.f));
}

void GLTFSample::LoadGLTF() {
	GLTFLoader loader;
	loader.Load(AssetProjectSetting::ToAssetPath("Models/DamagedHelmet/DamagedHelmet.gltf"));

}

