#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Component.h"
#include "Foundation/PreprocessorDirectives.h"

class Transform : public Component {
    DECLARE_CLASS(Transform);
public:
    static constexpr float kEpsilon = 0.00001f;
    enum TransformDirtyFlag {
        // clang-format off
		eLocalMatrix		= 1 << 0,
		eWorldMatrix		= 1 << 1,
		eInverseLocalMatrix = 1 << 2,
		eInverseWorldMatrix = 1 << 3,
        eAllDirty           = (eLocalMatrix | eWorldMatrix | eInverseLocalMatrix | eInverseWorldMatrix),
        // clang-format on
    };
    ENUM_FLAGS_AS_MEMBER(TransformDirtyFlag);
public:
    Transform();
public:
    void SetLocalPosition(const glm::vec3 &translate);
    void SetLocalScale(const glm::vec3 &scale);
    void SetLocalRotate(const glm::quat &rotate);
    void SetParent(Transform *pTransform);
    void AddChild(Transform *pTransform);
    void RemoveChild(Transform *pTransform);
    auto GetLocalMatrix() const -> const glm::mat4x4 &;
    auto GetWorldMatrix() const -> const glm::mat4x4 &;
    auto GetInverseLocalMatrix() const -> const glm::mat4x4 &;
    auto GetInverseWorldMatrix() const -> const glm::mat4x4 &;
    auto GetLocalPosition() const -> const glm::vec3 & {
        return _translation;
    }
    auto GetLocalScale() const -> const glm::vec3 & {
        return _scale;
    }
    auto GetLocalRotate() const -> const glm::quat & {
        return _rotation;
    }
    auto GetParent() const -> Transform * {
        return _pParent;
    }
    auto GetChildren() const -> const std::vector<Transform *> & {
        return _children;
    }
private:
    static void SetParentImpl(Transform *pParent, Transform *pChild);
    static void RemoveChildImpl(Transform *pParent, Transform *pChild);
private:
    // clang-format off
	glm::vec3					_translation;
	glm::quat					_rotation;
	glm::vec3					_scale;

	Transform				   *_pParent;
	std::vector<Transform *>	_children;

	mutable glm::mat4x4			_matLocal;
	mutable glm::mat4x4			_matWorld;
	mutable glm::mat4x4			_matInvLocal;
	mutable glm::mat4x4			_invMatWorld;
	mutable TransformDirtyFlag	_dirtyFlag;
    // clang-format on
};
