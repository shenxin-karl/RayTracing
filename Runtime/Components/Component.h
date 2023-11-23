#pragma once
#include "Object/Object.h"

class GameObject;
class Component : public Object {
    DECLARE_CLASS(Component);
public:
    Component();
public:
    auto GetGameObject() const -> GameObject * {
        return _pGameObject;
    }
public:
    virtual void OnAddToGameObject() {
    }
    virtual void OnRemoveFormGameObject() {
    }
private:
    friend class GameObject;
    void SetGameObject(GameObject *pGameObject);
private:
    GameObject *_pGameObject;
};
