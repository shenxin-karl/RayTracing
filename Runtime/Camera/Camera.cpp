#include "Camera.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include "Foundation/Exception.h"

Camera::Camera(const CameraDesc &desc) {
    glm::vec3 w = normalize(glm::vec3(desc.lookAt) - glm::vec3(desc.lookFrom));
    glm::vec3 u = normalize(cross(glm::vec3(desc.lookUp), w));
    glm::vec3 v = cross(w, u);

    _cameraData.lookFrom = desc.lookFrom;
    _cameraData.lookUp = v;
    _cameraData.lookAt = desc.lookAt;
    _cameraData.zNear = desc.nearClip;
    _cameraData.zFar = desc.farClip;
    _cameraData.fov = desc.fov;
    _cameraData.aspect = desc.aspect;

    Assert(_cameraData.fov > 1.f);
    Assert(_cameraData.zNear > 0.f);
    Assert(_cameraData.zFar > _cameraData.zNear);
}

void Camera::Update() {
    glm::mat4x4 view = glm::lookAt(_cameraData.lookFrom, _cameraData.lookAt, _cameraData.lookUp);
    glm::mat4x4 proj = glm::perspective(glm::radians(_cameraData.fov),
        _cameraData.aspect,
        _cameraData.zNear,
        _cameraData.zFar);
    glm::mat4x4 viewProj = proj * view;

    _cameraData.matView = view;
    _cameraData.matProj = proj;
    _cameraData.matViewProj = viewProj;
    _cameraData.matInvView = glm::inverse(view);
    _cameraData.matInvProj = glm::inverse(proj);
    _cameraData.matInvVewProj = glm::inverse(viewProj);
}
