#pragma once
#include <glm/mat4x4.hpp>
#include "Component.h"

class Camera : public Component {
	DECLARE_CLASS(Camera);
public:
	Camera();
	~Camera() override;
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
	CallbackHandle _preRenderHandle;
	CallbackHandle _resizeCallbackHandle;
	// clang-format on
};