#include "Camera.h"
#include <glm/ext/matrix_transform.hpp>
#include "Foundation/Logger.h"
#include "Object/GameObject.h"

Camera::Camera() {
    _aspect = 1.f;
    _zNear = 0.1f;
    _zFar = 1000.f;
    _fov = 45.f;
    _matView = glm::identity<glm::mat4x4>();
    _matProj = glm::identity<glm::mat4x4>();
    _matViewProj = glm::identity<glm::mat4x4>();
    _matInvView = glm::identity<glm::mat4x4>();
    _matInvProj = glm::identity<glm::mat4x4>();
    _matInvViewProj = glm::identity<glm::mat4x4>();
}

Camera::~Camera() {
}

void Camera::OnAddToScene() {
    Component::OnAddToScene();
    _preRenderHandle = GlobalCallbacks::Get().onPreRender.Register(this, &Camera::OnPreRender);
    _resizeCallbackHandle = GlobalCallbacks::Get().onResize.Register(this, &Camera::OnResize);
}

void Camera::OnRemoveFormScene() {
    Component::OnRemoveFormScene();
    _preRenderHandle.Release();
    _resizeCallbackHandle.Release();
}

void Camera::OnPreRender(GameTimer &timer) {
    Transform *pTransform = GetGameObject()->GetComponent<Transform>();
    if (pTransform == nullptr) {
        return;
    }

    glm::vec3 lookForm;
    glm::quat lookQuat;
    [[maybe_unused]] glm::vec3 scale;
    pTransform->GetWorldTRS(lookForm, lookQuat, scale);

    glm::mat3x3 rotationMatrix = glm::mat3_cast(lookQuat);
    glm::vec3 cameraUp = rotationMatrix[1];
    glm::vec3 cameraForward = rotationMatrix[2];

    _matView = glm::lookAt(lookForm, lookForm + cameraForward, cameraUp);
    _matProj = glm::perspective(glm::radians(_fov), _aspect, _zNear, _zFar);
    _matViewProj = _matProj * _matView;

    _matInvView = inverse(_matView);
    _matInvProj = inverse(_matProj);
    _matInvViewProj = inverse(_matViewProj);
}

void Camera::OnResize(size_t width, size_t height) {
    _aspect = static_cast<float>(width) / static_cast<float>(height);
}
