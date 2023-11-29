#include "GameObject.h"

GameObject::GameObject(): _sceneID(SceneID::Invalid) {
}

GameObject::~GameObject() {
	for (auto iter = _components.rbegin(); iter != _components.rend(); ++iter) {
		auto &pComponent = *iter;
		pComponent->OnRemoveFormGameObject();
	}
	_components.clear();
}

void GameObject::OnAddToScene(SceneID sceneID) {
	_sceneID = sceneID;
	for (std::unique_ptr<Component> &pComponent : _components) {
		pComponent->OnAddToScene();
	}

	Transform *pTransform = GetComponent<Transform>();
	if (pTransform == nullptr) {
		return;
	}
	for (Transform *pChild : pTransform->GetChildren()) {
		pChild->GetGameObject()->OnAddToScene(sceneID);
	}
}

void GameObject::OnRemoveFormScene() {
	Transform *pTransform = GetComponent<Transform>();
	if (pTransform == nullptr) {
		return;
	}
	for (Transform *pChild : pTransform->GetChildren()) {
		pChild->GetGameObject()->OnRemoveFormScene();
	}
	for (std::unique_ptr<Component> &pComponent : _components) {
		pComponent->OnRemoveFormScene();
	}
	_sceneID = SceneID::Invalid;
}

void GameObject::InitComponent(Component *pComponent) {
    pComponent->InitInstanceId();
    pComponent->SetGameObject(this);
    pComponent->OnAddToGameObject();
    if (_sceneID.IsValid()) {
        pComponent->OnAddToScene();
    }
}
