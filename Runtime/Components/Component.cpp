#include "Component.h"
#include "Object/GameObject.h"

Component::Component() : _pGameObject(nullptr) {
}

void Component::SetGameObject(GameObject *pGameObject) {
	_pGameObject = pGameObject;
}
