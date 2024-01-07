#include "Camera.h"
#include <glm/fwd.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Transform.h"
#include "Foundation/Logger.h"
#include "Object/GameObject.h"
#include "Renderer/RenderSetting.h"

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
    SetTickType(ePreRender);
}

Camera::~Camera() {
}

static std::vector<Camera *> sAvailableCameras;
auto Camera::GetAvailableCameras() -> const std::vector<Camera *> & {
    return sAvailableCameras;
}

void Camera::OnAddToScene() {
    Component::OnAddToScene();
    auto it = std::ranges::find(sAvailableCameras, this);
    if (it == sAvailableCameras.end()) {
	    sAvailableCameras.push_back(this);
    }
    _resizeCallbackHandle = GlobalCallbacks::Get().OnResize.Register(this, &Camera::OnResize);
}

void Camera::OnRemoveFormScene() {
    Component::OnRemoveFormScene();
    auto it = std::ranges::find(sAvailableCameras, this);
    if (it != sAvailableCameras.end()) {
	    sAvailableCameras.erase(it);
    }
    _resizeCallbackHandle.Release();
}

void Camera::OnPreRender() {
    Component::OnPreRender();
	Transform *pTransform = GetGameObject()->GetTransform();
    if (pTransform == nullptr) {
        return;
    }

    glm::vec3 lookForm;
    glm::quat lookQuat;
    [[maybe_unused]] glm::vec3 scale;
    pTransform->GetWorldTRS(lookForm, lookQuat, scale);

    auto &&[x, y, z] = glm::Quaternion2BasisAxis(lookQuat);

    glm::mat4x4 rotationMatrix = glm::mat4_cast(lookQuat);
    glm::mat4x4 translationMatrix = glm::translate(rotationMatrix, -lookForm);
    _matView = translationMatrix; glm::lookAt(lookForm, lookForm + z, glm::vec3(0, 1, 0));

    if (RenderSetting::Get().GetReversedZ()) {
		_matProj = glm::perspective(glm::radians(_fov), _aspect, _zFar, _zNear);
    } else {
		_matProj = glm::perspective(glm::radians(_fov), _aspect, _zNear, _zFar);
    }

    _matViewProj = _matProj * _matView;

    _matInvView = inverse(_matView);
    _matInvProj = inverse(_matProj);
    _matInvViewProj = inverse(_matViewProj);
}

void Camera::OnResize(size_t width, size_t height) {
    _aspect = static_cast<float>(width) / static_cast<float>(height);
    _screenWidth = static_cast<float>(width);
    _screenHeight = static_cast<float>(height);
}
