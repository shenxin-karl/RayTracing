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
private:
    void OnAddToScene() override;
    void OnRemoveFormScene() override;
    void OnPreRender(GameTimer &timer);
    void OnResize(size_t width, size_t height);
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
	CallbackHandle  _preRenderHandle;
	CallbackHandle  _resizeCallbackHandle;
    // clang-format on
};