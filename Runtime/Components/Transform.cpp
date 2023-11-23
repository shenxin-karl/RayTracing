#include "Transform.h"

Transform::Transform()
    : _translation(0.f),
      _rotation(1.f, 0.f, 0.f, 0.f),
      _scale(0.f),
      _pParent(nullptr),
      _matLocal(glm::identity<glm::mat4x4>()),
      _matWorld(glm::identity<glm::mat4x4>()),
      _matInvLocal(glm::identity<glm::mat4x4>()),
      _invMatWorld(glm::identity<glm::mat4x4>()),
      _dirtyFlag() {

    SetFlags(_dirtyFlag, eAllDirty);
}

void Transform::SetLocalPosition(const glm::vec3 &translate) {
    if (any(epsilonNotEqual(_translation, translate, kEpsilon))) {
        _translation = translate;
        SetFlags(_dirtyFlag, eAllDirty);
    }
}

void Transform::SetLocalScale(const glm::vec3 &scale) {
    if (any(epsilonNotEqual(_scale, scale, kEpsilon))) {
        _scale = scale;
        SetFlags(_dirtyFlag, eAllDirty);
    }
}

void Transform::SetLocalRotate(const glm::quat &rotate) {
    if (any(epsilonNotEqual(_rotation, rotate, kEpsilon))) {
        _rotation = rotate;
        SetFlags(_dirtyFlag, eAllDirty);
    }
}

void Transform::SetParent(Transform *pTransform) {
    if (_pParent != pTransform) {
	    if (_pParent != nullptr) {
		    RemoveChildImpl(_pParent, this);
	    }
        SetParentImpl(pTransform, this);
        SetFlags(_dirtyFlag, eWorldMatrix | eInverseWorldMatrix);
    }
}

void Transform::AddChild(Transform *pTransform) {
    if (pTransform->GetParent() != this) {
		pTransform->SetParent(this);
    }
}

void Transform::RemoveChild(Transform *pTransform) {
    if (pTransform->GetParent() == this) {
	    RemoveChildImpl(this, pTransform);
    }
}

auto Transform::GetLocalMatrix() const -> const glm::mat4x4 & {
    if (HasFlag(_dirtyFlag, eLocalMatrix)) {
        // TRS
        _matWorld = glm::scale(glm::mat4_cast(_rotation), _scale);
        _matWorld = glm::translate(glm::identity<glm::mat4>(), _translation) * _matWorld;
        ClearFlags(_dirtyFlag, eLocalMatrix);
    }
    return _matWorld;
}

auto Transform::GetWorldMatrix() const -> const glm::mat4x4 & {
    if (HasFlag(_dirtyFlag, eWorldMatrix)) {
        glm::mat4x4 parentMatrix = glm::identity<glm::mat4x4>();
        if (_pParent != nullptr) {
            parentMatrix = _pParent->GetWorldMatrix();
        }
        _matWorld = GetLocalMatrix() * parentMatrix;
        ClearFlags(_dirtyFlag, eWorldMatrix);
    }
    return _matWorld;
}

auto Transform::GetInverseLocalMatrix() const -> const glm::mat4x4 & {
    if (HasFlag(_dirtyFlag, eInverseLocalMatrix)) {
        _matInvLocal = inverse(GetLocalMatrix());
        ClearFlags(_dirtyFlag, eInverseLocalMatrix);
    }
    return _matInvLocal;
}

auto Transform::GetInverseWorldMatrix() const -> const glm::mat4x4 & {
    if (HasFlag(_dirtyFlag, eInverseWorldMatrix)) {
        _invMatWorld = inverse(GetWorldMatrix());
        ClearFlags(_dirtyFlag, eInverseWorldMatrix);
    }
    return _invMatWorld;
}

void Transform::SetParentImpl(Transform *pParent, Transform *pChild) {
    pParent->_children.push_back(pChild);
    pChild->_pParent = pParent;
}

void Transform::RemoveChildImpl(Transform *pParent, Transform *pChild) {
    for (auto it = pParent->_children.begin(); it != pParent->_children.end(); ++it) {
	    if (*it == pChild) {
            pChild->_pParent = nullptr;
		    pParent->_children.erase(it);
	    }
    }
}