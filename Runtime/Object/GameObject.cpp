#include "GameObject.h"
#include "Components/Transform.h"

GameObject::GameObject() : _active(true), _sceneID(SceneID::Invalid) {
}

GameObject::~GameObject() {
    for (auto iter = _children.rbegin(); iter != _children.rend(); ++iter) {
	    auto &pChild = *iter;
        pChild->GetTransform()->SetParent(nullptr);
    }
    _children.clear();
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
    if (pChild->GetTransform()->GetParent()->GetGameObject() != this) {
        _children.push_back(pChild);
        GetTransform()->AddChild(pChild->GetTransform());
    }
}

void GameObject::RemoveChild(GameObject *pChild) {
    auto iter = _children.begin();
    while (iter != _children.end()) {
        if (iter->Get() == pChild) {
            if (_sceneID.IsValid()) {
	            (*iter)->OnRemoveFormScene();
            }
            (*iter)->GetTransform()->SetParent(nullptr);
            _children.erase(iter);
            return;
        }
        ++iter;
    }
}

auto GameObject::GetChildren() const -> const ChildrenContainer & {
    return _children;
}

auto GameObject::GetTransform() -> Transform * {
    Assert(_components.size() >= 1);
    Assert(_components.front()->GetClassTypeID() == GetTypeID<Transform>());
    return static_cast<Transform *>(_components.front().Get());
}

auto GameObject::GetTransform() const -> const Transform * {
    return const_cast<GameObject *>(this)->GetTransform();
}

void GameObject::InitComponent(Component *pComponent) {
    pComponent->InitInstanceId();
    pComponent->SetGameObject(this);
    pComponent->OnAddToGameObject();
    if (_sceneID.IsValid()) {
        pComponent->OnAddToScene();
    }
}

bool GameObject::GetParentActive() const {
    const Transform *pTransform = GetTransform();
    const Transform *pParent = pTransform->GetParent();
    if (pParent == nullptr) {
        return true;
    }
    return pParent->GetGameObject()->GetActive();
}

void GameObject::InvokeTickFunc(Component::TickType tickType, void(GameObject::*pTickFunc)(), void(Component::*pComponentTickFunc)()) {
    for (SharedPtr<Component> &pComponent : _components) {
        if (HasFlag(pComponent->GetTickType(), tickType)) {
            (pComponent.Get()->*pComponentTickFunc)();
        }
    }
    for (SharedPtr<GameObject> &pChild : _children) {
        if (pChild->_active) {
            (pChild.Get()->*pTickFunc)();
        }
    }
}

void GameObject::InnerOnPreUpdate() {
    InvokeTickFunc(Component::ePreUpdate, &GameObject::InnerOnPreUpdate, &Component::InnerOnPreUpdate);
}

void GameObject::InnerOnUpdate() {
    InvokeTickFunc(Component::eUpdate, &GameObject::InnerOnUpdate, &Component::InnerOnUpdate);
}

void GameObject::InnerOnPostUpdate() {
    InvokeTickFunc(Component::ePostUpdate, &GameObject::InnerOnPostUpdate, &Component::InnerOnPostUpdate);
}

void GameObject::InnerOnPreRender() {
    InvokeTickFunc(Component::ePreRender, &GameObject::InnerOnPreRender, &Component::InnerOnPreRender);
}

void GameObject::InnerOnRender() {
    InvokeTickFunc(Component::eRender, &GameObject::InnerOnRender, &Component::InnerOnRender);
}

void GameObject::InnerOnPostRender() {
    InvokeTickFunc(Component::ePostRender, &GameObject::InnerOnPostRender, &Component::InnerOnPostRender);
}
