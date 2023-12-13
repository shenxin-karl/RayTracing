#include "MeshRenderer.h"

void MeshRenderer::SetMesh(std::shared_ptr<Mesh> pMesh) {
	_pMesh = std::move(pMesh);
}

void MeshRenderer::SetMaterial(std::shared_ptr<StandardMaterial> pMaterial) {
	_pMaterial = std::move(pMaterial);
}

void MeshRenderer::OnRemoveFormGameObject() {
	Component::OnRemoveFormGameObject();
}

void MeshRenderer::OnRemoveFormScene() {
	Component::OnRemoveFormScene();
}
