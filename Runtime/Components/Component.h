#pragma once
#include "Foundation/PreprocessorDirectives.h"
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
public:
    // clang-format off
    enum TickType {
        eNone           = 0,
	    ePreUpdate      = 1 << 0,     
        eUpdate         = 1 << 1,
        ePostUpdate     = 1 << 2,
        ePreRender      = 1 << 3,
        eRender         = 1 << 4,
        ePostRender     = 1 << 5
    };
    ENUM_FLAGS_AS_MEMBER(TickType);
    // clang-format on
    auto GetTickType() const -> TickType {
	    return _tickType;
    }
    void SetTickType(TickType tickType) {
	    _tickType = tickType;
    }
    virtual bool GetActive() const {
	    return true;
    }
    // To enable the callback, you must set tickType
    virtual void OnPreUpdate() {}
    virtual void OnUpdate() {}
    virtual void OnPostUpdate() {}
    virtual void OnPreRender() {}
    virtual void OnRender() {}
    virtual void OnPostRender() {}
private:
    friend class GameObject;
    void SetGameObject(GameObject *pGameObject);
    void InnerOnPreUpdate();
    void InnerOnUpdate();
    void InnerOnPostUpdate();
    void InnerOnPreRender();
    void InnerOnRender();
    void InnerOnPostRender();
private:
    // clang-format off
    GameObject *_pGameObject;
    TickType    _tickType;
    // clang-format on
};
