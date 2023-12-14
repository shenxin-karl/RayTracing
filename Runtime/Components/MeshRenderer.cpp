#include "MeshRenderer.h"
#include "Object/GameObject.h"
#include "SceneObject/SceneManager.h"

MeshRenderer::MeshRenderer() : _pCurrentScene(nullptr) {
	SetTickType(ePreRender);
}

void MeshRenderer::SetMesh(std::shared_ptr<Mesh> pMesh) {
	_pMesh = std::move(pMesh);
}

void MeshRenderer::SetMaterial(std::shared_ptr<StandardMaterial> pMaterial) {
	_pMaterial = std::move(pMaterial);
}

void MeshRenderer::OnRemoveFormScene() {
	Component::OnRemoveFormScene();
	_pCurrentScene = nullptr;
}

void MeshRenderer::OnAddToScene() {
	Component::OnAddToScene();
	SceneID sceneId = GetGameObject()->GetSceneID();
	_pCurrentScene = SceneManager::GetInstance()->GetScene(sceneId);
}

void MeshRenderer::OnPreRender() {
	Component::OnPreRender();

}
