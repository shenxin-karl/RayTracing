#include "GameObject.h"

GameObject::~GameObject() {
	for (auto iter = _components.rbegin(); iter != _components.rbegin(); ++iter) {
		auto &pComponent = *iter;
		pComponent->OnRemoveFormGameObject();
	}
	_components.clear();
}
