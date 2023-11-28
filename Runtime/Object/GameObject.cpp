#include "GameObject.h"

GameObject::GameObject(): _sceneID(-1) {
}

GameObject::~GameObject() {
	for (auto iter = _components.rbegin(); iter != _components.rbegin(); ++iter) {
		auto &pComponent = *iter;
		pComponent->OnRemoveFormGameObject();
	}
	_components.clear();
}

void GameObject::OnAddToScene(int32_t sceneID) {
	_sceneID = sceneID;
	for (std::unique_ptr<Component> &pComponent : _components) {
		pComponent->OnAddToScene();
	}
}

void GameObject::OnRemoveFormScene() {
	for (std::unique_ptr<Component> &pComponent : _components) {
		pComponent->OnRemoveFormScene();
	}
	_sceneID = -1;
}
