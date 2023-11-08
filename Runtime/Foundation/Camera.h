#pragma once
#include <glm/glm.hpp>

struct CameraData {
	// clang-format off
	float		fov;
	float		aspect;
	float		zNear;
	float		zFar;
	glm::vec3	lookForm;
	glm::vec3	lookUp;
	glm::vec3	lookAt;
	glm::mat4x4 matView;
	glm::mat4x4 matInvView;
	glm::mat4x4 matProj;
	glm::mat4x4 matInvProj;
	glm::mat4x4 matViewProj;
	glm::mat4x4 matInvVewProj;
	// clang-format on
};

struct CameraDesc {
	// clang-format off
	glm::vec3	lookFrom;
	glm::vec3	lookUp;
	glm::vec3	lookAt;
	float       fov;
	float       nearClip;
	float       farClip;
	float       aspect;
	// clang-format on
};

class Camera {
public:
	Camera(const CameraDesc &desc);
	void Update();
	void SetLookForm(const glm::vec3 &lookForm) {
		_cameraData.lookForm = lookForm;
	}
	void SetLookUp(const glm::vec3 &lookUp) {
		_cameraData.lookUp = lookUp;
	}
	void SetLookAt(const glm::vec3 &lookAt) {
		_cameraData.lookAt = lookAt;
	}
	void SetFov(float fov) {
		_cameraData.fov = fov;
	}
	void SetAspect(float aspect) {
		_cameraData.aspect = aspect;
	}
	auto GetCameraData() const -> const CameraData & {
		return _cameraData;
	}
private:
	CameraData _cameraData;
};