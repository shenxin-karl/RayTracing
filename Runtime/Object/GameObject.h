#pragma once
#include "Object.h"
#include "Components/Component.h"
#include "Components/Transform.h"

class Component;
class GameObject : public Object {
    DECLARE_CLASS(GameObject);
private:
    GameObject();
public:
    ~GameObject() override;
    void OnAddToScene(int32_t sceneID);
    void OnRemoveFormScene();
public:
    static auto Create() -> std::shared_ptr<GameObject> {
        struct MakeGameObject : public GameObject {
            using GameObject::GameObject;
        };
        std::shared_ptr<GameObject> pGameObject = std::make_shared<MakeGameObject>();
        pGameObject->InitInstanceId();
        pGameObject->AddComponent<Transform>();
        return pGameObject;
    }

    template<typename T>
        requires(std::is_base_of_v<Component, T>)
    auto GetComponent() -> T * {
        for (std::unique_ptr<Component> &pComponent : _components) {
            if (::GetTypeID<T>() == pComponent->GetClassTypeID()) {
                return static_cast<T *>(pComponent.get());
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
        auto &pComponent = _components.emplace_back(std::make_unique<T>());
        pComponent->InitInstanceId();
        pComponent->SetGameObject(this);
        pComponent->OnAddToGameObject();
        return static_cast<T *>(pComponent.get());
    }

    template<typename T>
        requires(std::is_base_of_v<Component, T>)
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
        for (auto it = _components.begin(); it != _components.end(); ++it) {
            if ((*it)->GetClassTypeID() == typeId) {
                return true;
            }
        }
		return false;
    }
    auto GetSceneID() const -> int32_t {
	    return _sceneID;
    }
private:
    using ComponentContainer = std::vector<std::unique_ptr<Component>>;
    friend class Scene;
    void SetSceneID(int sceneID) {
	    _sceneID = sceneID;
    }
private:
    // clang-format off
    int32_t            _sceneID;
    ComponentContainer _components;
    // clang-format on
};
