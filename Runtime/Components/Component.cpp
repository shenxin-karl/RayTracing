#include "Component.h"

Component::Component() : _pGameObject(nullptr) {

}

void Component::SetGameObject(GameObject *pGameObject) {
	_pGameObject = pGameObject;
}
