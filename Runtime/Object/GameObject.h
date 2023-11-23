#pragma once
#include "Object.h"
#include "Components/Component.h"

class Component;
class GameObject : public Object {
	DECLARE_CLASS(GameObject);
private:
	GameObject() = default;
public:
	static auto Create() -> std::shared_ptr<GameObject> {
		std::shared_ptr<GameObject> pGameObject = std::make_shared<GameObject>();
		pGameObject->InitInstanceId();
		return pGameObject;
	}

	template<typename T> requires(std::is_base_of_v<Component, T>)
	auto GetComponent() -> T * {
		for (std::unique_ptr<Component> &pComponent : _components) {
			if (GetTypeIndex<T>() == pComponent->GetTypeIndex()) {
				return pComponent.get();
			}
		}
		return nullptr;
	}

	template<typename T> requires(std::is_base_of_v<Component, T>)
	auto AddComponent() -> T * {
		if (Component *pComponent = GetComponent<T>()) {
			return pComponent;
		}
		auto &pComponent = _components.emplace_back(std::make_unique<T>());
		pComponent->InitInstanceId();
		pComponent->SetGameObject(this);
		pComponent->OnAddToGameObject();
		return pComponent.get();
	}

	template<typename T> requires(std::is_base_of_v<Component, T>)
	bool RemoveComponent() {
		for (auto it = _components.begin(); it != _components.end(); ++it) {
			if ((*it)->GetTypeIndex() == GetTypeIndex<T>()) {
				auto &pComponent = *it;
				pComponent->OnRemoveFormGameObject();
				_components.erase(it);
				return true;
			}
		}
		return false;
	}
private:
	using ComponentContainer = std::vector<std::unique_ptr<Component>>;
	ComponentContainer _components;
};
