#pragma once
#include "Object/Object.h"
#include "Utils/GlobalCallbacks.h"

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
    virtual void OnAddToScene() {
    }
    virtual void OnRemoveFormScene() {
    }
private:
    friend class GameObject;
    void SetGameObject(GameObject *pGameObject);
private:
    GameObject *_pGameObject;
};
