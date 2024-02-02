#include "RenderView.h"
#include "RenderSetting.h"
#include "Components/Light.h"
#include "Components/Transform.h"
#include "Foundation/GameTimer.h"
#include "Object/GameObject.h"

RenderView::RenderView() : _isFirstFrame(true), _cbPrePass(), _cbLighting() {
}

void RenderView::Step0_OnNewFrame() {
    UpdatePreviousFrameData();
}

void RenderView::Step1_UpdateCameraMatrix(const Camera *pCamera) {
    _cbPrePass.matView = pCamera->GetViewMatrix();
    _cbPrePass.matInvView = pCamera->GetInverseViewMatrix();
    _cbPrePass.matProj = pCamera->GetProjectionMatrix();
    _cbPrePass.matInvProj = pCamera->GetInverseProjectionMatrix();
    _cbPrePass.matViewProj = pCamera->GetViewProjectionMatrix();
    _cbPrePass.matInvViewProj = pCamera->GetInverseViewProjectionMatrix();

    _cbPrePass.nearClip = pCamera->GetNearClip();
    _cbPrePass.farClip = pCamera->GetFarClip();

    _cbPrePass.radianFov = glm::radians(pCamera->GetFov());
    _cbPrePass.degreeFov = pCamera->GetFov();

    if (RenderSetting::Get().GetReversedZ()) {
        _cbPrePass.zBufferParams.x = -1.0 + _cbPrePass.farClip / _cbPrePass.nearClip;
        _cbPrePass.zBufferParams.y = 1.f;
    } else {
        _cbPrePass.zBufferParams.x = 1.0 - _cbPrePass.farClip / _cbPrePass.nearClip;
        _cbPrePass.zBufferParams.y = _cbPrePass.farClip / _cbPrePass.nearClip;
    }
    _cbPrePass.zBufferParams.z = _cbPrePass.zBufferParams.x / _cbPrePass.farClip;
    _cbPrePass.zBufferParams.w = _cbPrePass.zBufferParams.y / _cbPrePass.farClip;

    glm::vec3 right;
    glm::vec3 up;
    glm::vec3 forward;
    glm::quat rotation = glm::quat_cast(_cbPrePass.matView);
    glm::Quaternion2BasisAxis(rotation, right, up, forward);

    glm::vec3 cameraPos = -glm::vec3(_cbPrePass.matView[3][0], _cbPrePass.matView[3][1], _cbPrePass.matView[3][2]);
    _cbPrePass.cameraPos = cameraPos;
    _cbPrePass.cameraLookUp = up;
    _cbPrePass.cameraLookAt = forward;
}

void RenderView::Step2_UpdateResolutionInfo(const ResolutionInfo &resolution) {
    _cbPrePass.cameraJitter = {resolution.jitterX, resolution.jitterY};
    _cbPrePass.viewportJitter = {
        resolution.jitterX * +2.f / resolution.renderWidth,
        resolution.jitterY * -2.f / resolution.renderHeight,
    };
    _cbPrePass.renderSize = {resolution.renderWidth, resolution.renderHeight};
    _cbPrePass.displaySize = {resolution.displayWidth, resolution.displayHeight};
    _cbPrePass.invRenderSize = 1.f / _cbPrePass.renderSize;
    _cbPrePass.invDisplaySize = 1.f / _cbPrePass.displaySize;
}

void RenderView::Step3_UpdateDirectionalLightInfo(const DirectionalLight *pDirectionalLight) {
    _cbLighting.directionalLight.color = pDirectionalLight->GetColor();
    _cbLighting.directionalLight.intensity = pDirectionalLight->GetIntensity();
    glm::quat worldRotation = pDirectionalLight->GetGameObject()->GetTransform()->GetWorldRotation();
    glm::vec3 x, y, z;
    Quaternion2BasisAxis(worldRotation, x, y, z);
    _cbLighting.directionalLight.direction = -z;
}

void RenderView::BeforeFinalizeOptional_SetMipBias(float mipBias) {
    _cbPrePass.mipBias = mipBias;
}

void RenderView::Step4_Finalize() {
    glm::mat4x4 jitterMatrix = glm::translate(glm::identity<glm::mat4>(), glm::vec3(_cbPrePass.viewportJitter, 0.f));
    _cbPrePass.matJitteredViewProj = jitterMatrix * _cbPrePass.matViewProj;
    _cbPrePass.matInvJitteredViewProj = glm::inverse(_cbPrePass.matJitteredViewProj);
    _cbPrePass.matJitteredProj = jitterMatrix * _cbPrePass.matProj;
    _cbPrePass.matInvJitteredProj = glm::inverse(_cbPrePass.matJitteredProj);

    _cbPrePass.totalTime = GameTimer::Get().GetTotalTimeS();
    _cbPrePass.deltaTime = GameTimer::Get().GetDeltaTimeS();

    RenderSetting &renderSetting = RenderSetting::Get();
    _cbLighting.ambientLight.color = renderSetting.GetAmbientColor();
    _cbLighting.ambientLight.intensity = renderSetting.GetAmbientIntensity();

    if (_isFirstFrame) {
        UpdatePreviousFrameData();
        _isFirstFrame = false;
    }
}

void RenderView::UpdatePreviousFrameData() {
    _cbPrePass.matViewProjPrev = _cbPrePass.matViewProj;
    _cbPrePass.matJitterViewProjPrev = _cbPrePass.matJitteredViewProj;
    _cbPrePass.viewportJitterPrev = _cbPrePass.viewportJitter;
    _cbPrePass.cameraJitterPrev = _cbPrePass.cameraJitter;
}
