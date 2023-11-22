#pragma once
#include "Object.h"
#include "Components/Component.h"

class Component;

class GameObject : public Object {
public:
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
		_components.push_back(std::make_unique<T>());
		return _components.back().get();
	}

	template<typename T> requires(std::is_base_of_v<Component, T>)
	bool RemoveComponent() {
		for (auto it = _components.begin(); it != _components.end(); ++it) {
			if ((*it)->GetTypeIndex() == GetTypeIndex<T>()) {
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
