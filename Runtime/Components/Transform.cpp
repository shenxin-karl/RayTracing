#include "Transform.h"
#include <glm/gtx/matrix_decompose.hpp>

Transform::Transform()
    : _translation(0.f),
      _rotation(1.f, 0.f, 0.f, 0.f),
      _scale(1.f),
      _pParent(nullptr),
      _worldTranslation(0.f),
      _worldRotation(1.f, 0.f, 0.f, 0.f),
      _worldScale(1.f),
      _matLocal(glm::identity<glm::mat4x4>()),
      _matWorld(glm::identity<glm::mat4x4>()),
      _matInvLocal(glm::identity<glm::mat4x4>()),
      _matInvWorld(glm::identity<glm::mat4x4>()),
      _dirtyFlag() {
    _dirtyFlag = SetFlags(_dirtyFlag, eAllDirty);
}

Transform::~Transform() {
    if (_pParent != nullptr) {
		SetParent(nullptr);
    }

    for (auto it = _children.begin(); it != _children.end(); ++it) {
        auto *pChild = *it;
        pChild->_pParent = nullptr;
        pChild->_dirtyFlag = SetFlags(pChild->_dirtyFlag, eWorldAttribute);
    }
    _children.clear();
}

void Transform::SetLocalPosition(const glm::vec3 &translate) {
    if (any(epsilonNotEqual(_translation, translate, kEpsilon))) {
        _translation = translate;
        _dirtyFlag = SetFlags(_dirtyFlag, eAllDirty);
        MakeChildrenDirty(eWorldAttribute);
    }
}

void Transform::SetLocalScale(const glm::vec3 &scale) {
    if (any(epsilonNotEqual(_scale, scale, kEpsilon))) {
        _scale = scale;
        _dirtyFlag = SetFlags(_dirtyFlag, eAllDirty);
        MakeChildrenDirty(eWorldAttribute);
    }
}

void Transform::SetLocalRotation(const glm::quat &rotate) {
    if (any(epsilonNotEqual(_rotation, rotate, kEpsilon))) {
        _rotation = rotate;
        _dirtyFlag = SetFlags(_dirtyFlag, eAllDirty);
        MakeChildrenDirty(eWorldAttribute);
    }
}

void Transform::SetLocalMatrix(const glm::mat4x4 &matrix) {
    _matLocal = matrix;
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(_matLocal, _scale, _rotation, _translation, skew, perspective);
    _dirtyFlag = ClearFlags(_dirtyFlag, eLocalMatrix);
    _dirtyFlag = SetFlags(_dirtyFlag, eInverseLocalMatrix | eWorldAttribute);
    MakeChildrenDirty(eWorldAttribute);
}

void Transform::SetWorldMatrix(const glm::mat4x4 &matrix) {
    _matWorld = matrix;

    glm::mat4x4 parentInvWorld = glm::identity<glm::mat4x4>();
    if (_pParent != nullptr) {
        parentInvWorld = _pParent->GetInverseWorldMatrix();
    }
    _matLocal = matrix * parentInvWorld;

    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(_matLocal, _scale, _rotation, _translation, skew, perspective);

    MakeChildrenDirty(eWorldAttribute);
    _dirtyFlag = SetFlags(_dirtyFlag, eInverseLocalMatrix | eWorldAttribute);
    _dirtyFlag = ClearFlags(_dirtyFlag, eWorldMatrix | eLocalMatrix);
}

void Transform::SetLocalTRS(const glm::vec3 &translation, const glm::quat &rotation, const glm::vec3 &scale) {
    _translation = translation;
    _rotation = rotation;
    _scale = scale;
    _dirtyFlag = SetFlags(_dirtyFlag, eInverseLocalMatrix | eLocalMatrix | eWorldAttribute);
    MakeChildrenDirty(eWorldAttribute);
}

void Transform::SetWorldTRS(const glm::vec3 &translation, const glm::quat &rotation, const glm::vec3 &scale) {
    SetWorldMatrix(MakeAffineMatrix(translation, rotation, scale));
}

void Transform::SetParent(Transform *pTransform) {
    if (_pParent != pTransform) {
        if (_pParent != nullptr) {
            RemoveChildImpl(_pParent, this);
        }
        SetParentImpl(pTransform, this);
        MakeChildrenDirty(eWorldAttribute);
        _dirtyFlag = SetFlags(_dirtyFlag, eWorldAttribute);
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
        _dirtyFlag = SetFlags(pTransform->_dirtyFlag, eWorldAttribute);
    }
}

auto Transform::GetLocalMatrix() const -> const glm::mat4x4 & {
    if (HasFlag(_dirtyFlag, eLocalMatrix)) {
        _matWorld = MakeAffineMatrix(_translation, _rotation, _scale);
        _dirtyFlag = ClearFlags(_dirtyFlag, eLocalMatrix);
    }
    return _matWorld;
}

auto Transform::GetWorldMatrix() const -> const glm::mat4x4 & {
    if (HasFlag(_dirtyFlag, eWorldMatrix)) {
        glm::mat4x4 parentMatrix = glm::identity<glm::mat4x4>();
        if (_pParent != nullptr) {
            parentMatrix = _pParent->GetWorldMatrix();
        }
        _matWorld = parentMatrix * GetLocalMatrix();
        _dirtyFlag = ClearFlags(_dirtyFlag, eWorldMatrix);
    }
    return _matWorld;
}

auto Transform::GetInverseLocalMatrix() const -> const glm::mat4x4 & {
    if (HasFlag(_dirtyFlag, eInverseLocalMatrix)) {
        _matInvLocal = inverse(GetLocalMatrix());
        _dirtyFlag = ClearFlags(_dirtyFlag, eInverseLocalMatrix);
    }
    return _matInvLocal;
}

auto Transform::GetInverseWorldMatrix() const -> const glm::mat4x4 & {
    if (HasFlag(_dirtyFlag, eInverseWorldMatrix)) {
        _matInvWorld = inverse(GetWorldMatrix());
        _dirtyFlag = ClearFlags(_dirtyFlag, eInverseWorldMatrix);
    }
    return _matInvWorld;
}

auto Transform::GetWorldPosition() const -> const glm::vec3 & {
    ConditionUpdateWorldAttribute();
    return _worldTranslation;
}

auto Transform::GetWorldScale() const -> const glm::vec3 & {
    ConditionUpdateWorldAttribute();
    return _worldScale;
}

auto Transform::GetWorldRotation() const -> const glm::quat & {
    ConditionUpdateWorldAttribute();
    return _worldRotation;
}

void Transform::GetLocalTRS(glm::vec3 &translation, glm::quat &rotation, glm::vec3 &scale) const {
    translation = _translation;
    rotation = _rotation;
    scale = _scale;
}

void Transform::GetWorldTRS(glm::vec3 &translation, glm::quat &rotation, glm::vec3 &scale) const {
    ConditionUpdateWorldAttribute();
    translation = _worldTranslation;
    rotation = _worldRotation;
    scale = _worldScale;
}

glm::mat4x4 Transform::MakeAffineMatrix(const glm::vec3 &translation, const glm::quat &rotation, const glm::vec3 &scale) {
	glm::mat4x4 matrix = glm::scale(glm::mat4_cast(rotation), scale);
    matrix = glm::translate(glm::identity<glm::mat4>(), translation) * matrix;
    return matrix;
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

void Transform::ConditionUpdateWorldAttribute() const {
    if (HasAnyFlags(_dirtyFlag, eWorldTranslation | eWorldScale | eWorldRotation)) {
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(GetWorldMatrix(), _worldScale, _worldRotation, _worldTranslation, skew, perspective);
        _dirtyFlag = ClearFlags(_dirtyFlag, eWorldTranslation | eWorldScale | eWorldRotation);
    }
}

void Transform::MakeChildrenDirty(TransformDirtyFlag flag) {
    for (auto *pChild : _children) {
        pChild->_dirtyFlag = SetFlags(pChild->_dirtyFlag, flag);
    }
}
