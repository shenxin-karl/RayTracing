#include "Component.h"
#include "Object/GameObject.h"

Component::Component() : _pGameObject(nullptr), _tickType(eNone) {
}

void Component::SetGameObject(GameObject *pGameObject) {
    _pGameObject = pGameObject;
}

void Component::InnerOnPreUpdate() {
    if (GetActive()) {
        OnPreUpdate();
    }
}

void Component::InnerOnUpdate() {
    if (GetActive()) {
        OnUpdate();
    }
}

void Component::InnerOnPostUpdate() {
    if (GetActive()) {
        OnPostUpdate();
    }
}

void Component::InnerOnPreRender() {
    if (GetActive()) {
        OnPreRender();
    }
}

void Component::InnerOnRender() {
    if (GetActive()) {
        OnRender();
    }
}

void Component::InnerOnPostRender() {
    if (GetActive()) {
        OnPostRender();
    }
}
