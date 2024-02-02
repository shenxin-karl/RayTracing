#pragma once
#include <glm/mat4x4.hpp>
#include "Component.h"

class Camera : public Component {
    DECLARE_CLASS(Camera);
public:
    Camera();
    ~Camera() override;
public:
	static auto GetAvailableCameras() -> const std::vector<Camera *> &;
	auto GetViewMatrix() const -> const glm::mat4x4 & {
		return _matView;
	}
	auto GetProjectionMatrix() const -> const glm::mat4x4 & {
		return _matProj;
	}
	auto GetViewProjectionMatrix() const -> const glm::mat4x4 & {
		return _matViewProj;
	}
	auto GetInverseViewMatrix() const -> const glm::mat4x4 & {
		return _matInvView;
	}
	auto GetInverseProjectionMatrix() const -> const glm::mat4x4 & {
		return _matInvProj;
	}
	auto GetInverseViewProjectionMatrix() const -> const glm::mat4x4 & {
		return _matInvViewProj;
	}
	auto GetNearClip() const -> float {
		return _zNear;
	}
	auto GetFarClip() const -> float {
		return _zFar;
	}
	// return degree
	auto GetFov() const -> float {
		return _fov;
	}
	void SetNearClip(float nearClip) {
		_zNear = nearClip;
	}
	void SetFarClip(float farClip) {
		_zFar = farClip;
	}
	void SetAspect(float aspect) {
		_aspect = aspect;
	}
	void SetAspect(float width, float height) {
		_aspect = width / height;
	}
private:
    void OnAddToScene() override;
    void OnRemoveFormScene() override;
    void OnPreRender() override;
private:
    // clang-format off
	float			_aspect;
	float			_zNear;
	float			_zFar;
	float			_fov;
	glm::mat4x4		_matView;
	glm::mat4x4		_matProj;
	glm::mat4x4		_matViewProj;
	glm::mat4x4		_matInvView;
	glm::mat4x4		_matInvProj;
	glm::mat4x4		_matInvViewProj;
    // clang-format on
};