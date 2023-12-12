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
	for (SharedPtr<Component> &pComponent : _components) {
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
	for (SharedPtr<Component> &pComponent : _components) {
		pComponent->OnRemoveFormScene();
	}
	_sceneID = SceneID::Invalid;
	_children.clear();
}

auto GameObject::Create() -> SharedPtr<GameObject> {
    struct MakeGameObject : public GameObject {
        using GameObject::GameObject;
    };
    SharedPtr<GameObject> pGameObject = MakeShared<MakeGameObject>();
    pGameObject->InitInstanceId();
    pGameObject->AddComponent<Transform>();
    return pGameObject;
}


void GameObject::AddChild(SharedPtr<GameObject> pChild) {

}

void GameObject::RemoveChild(GameObject *pChild) {
	auto iter = _children.begin();
	while (iter != _children.end()) {
		if (iter->Get() == pChild) {
			(*iter)->OnRemoveFormScene();
			_children.erase(iter);
			return;
		}
		++iter;
	}
}

auto GameObject::GetChildren() const -> const ChildrenContainer & {
	return _children;
}

void GameObject::InitComponent(Component *pComponent) {
    pComponent->InitInstanceId();
    pComponent->SetGameObject(this);
    pComponent->OnAddToGameObject();
    if (_sceneID.IsValid()) {
        pComponent->OnAddToScene();
    }
}
