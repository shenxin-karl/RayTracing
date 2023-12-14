#pragma once
#include "Object.h"
#include "Components/Component.h"
#include "SceneObject/SceneID.hpp"
#include "Foundation/Exception.h"
#include "Foundation/Memory/SharedPtr.hpp"

class Component;
class Transform;
class GameObject : public Object {
    DECLARE_CLASS(GameObject);
private:
    using ComponentContainer = std::vector<SharedPtr<Component>>;
    using ChildrenContainer = std::vector<SharedPtr<GameObject>>;
    friend class Scene;
    GameObject();
public:
    ~GameObject() override;
    void OnAddToScene(SceneID sceneID);
    void OnRemoveFormScene();
public:
    static auto Create() -> SharedPtr<GameObject>;

    template<typename T>
        requires(std::is_base_of_v<Component, T>)
    auto GetComponent() -> T * {
        for (SharedPtr<Component> &pComponent : _components) {
            if (::GetTypeID<T>() == pComponent->GetClassTypeID()) {
                return static_cast<T *>(pComponent.Get());
            }
        }
        return nullptr;
    }

    template<typename T>
        requires(std::is_base_of_v<Component, T>)
    auto GetComponent() const -> const T * {
        for (const SharedPtr<Component> &pComponent : _components) {
            if (::GetTypeID<T>() == pComponent->GetClassTypeID()) {
                return static_cast<const T *>(pComponent.Get());
            }
        }
        return nullptr;
    }

    template<typename T>
        requires(std::is_base_of_v<Component, T>)
    auto AddComponent() -> T * {
        if (T *pComponent = GetComponent<T>()) {
            return pComponent;
        }
        auto &pComponent = _components.emplace_back(MakeShared<T>());
		InitComponent(pComponent.Get());
        return static_cast<T *>(pComponent.Get());
    }

    template<typename T>
		requires(std::is_base_of_v<Component, T>)
    bool AddComponent(SharedPtr<T> &&pComponent) {
        Assert(pComponent != nullptr);
	    if (GetComponent<T>() != nullptr) {
		    return false;
	    }
        if (pComponent->GetGameObject() != nullptr) {
	        return false;
        }
        _components.emplace_back(std::move(pComponent));
		InitComponent(_components.back().Get());
        return true;
    }

    template<typename T>
        requires(std::is_base_of_v<Component, T> && !std::is_same_v<T, Transform>)
    bool RemoveComponent() {
        for (auto it = _components.begin(); it != _components.end(); ++it) {
            if ((*it)->GetClassTypeID() == GetTypeID<T>()) {
                auto &pComponent = *it;
                pComponent->OnRemoveFormGameObject();
                _components.erase(it);
                return true;
            }
        }
        return false;
    }

    bool HasComponent(TypeID typeId) const {
        Assert(typeId != GetTypeID<Transform>());
        for (auto it = _components.begin(); it != _components.end(); ++it) {
            if ((*it)->GetClassTypeID() == typeId) {
                return true;
            }
        }
		return false;
    }
    auto GetSceneID() const -> SceneID {
	    return _sceneID;
    }

    void SetActive(bool active) {
	    _active = active;
    }
	bool GetActive() const {
	    return _active && GetParentActive();
    }

    void AddChild(SharedPtr<GameObject> pChild);
    void RemoveChild(GameObject *pChild);
    auto GetChildren() const -> const ChildrenContainer &;
    auto GetTransform() -> Transform *;
    auto GetTransform() const -> const Transform *;
private:
    void SetSceneID(SceneID sceneID) {
	    _sceneID = sceneID;
    }
    void InitComponent(Component *pComponent);
    bool GetParentActive() const;
private:
    void InvokeTickFunc(Component::TickType tickType, void (GameObject::*pTickFunc)(), void (Component::*pComponentTickFunc)());
	void InnerOnPreUpdate();
    void InnerOnUpdate();
    void InnerOnPostUpdate();
    void InnerOnPreRender();
    void InnerOnRender();
    void InnerOnPostRender();
private:
    // clang-format off
    bool               _active;
    SceneID            _sceneID;
    ComponentContainer _components;
    ChildrenContainer  _children; 
    // clang-format on
};
