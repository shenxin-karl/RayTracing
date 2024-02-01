#include "ConstantBufferHelper.h"
#include "Components/Camera.h"
#include "D3d12/Context.h"
#include "Object/GameObject.h"
#include "Components/Transform.h"
#include "Foundation/GameTimer.h"
#include "SceneObject/SceneLightManager.h"
#include "Components/Light.h"
#include "Renderer/RenderSetting.h"

namespace cbuffer {

auto MakeCbPrePass(const CameraState *pCurrentCameraState, const CameraState *pPreviousCameraState) -> CbPrePass {
    CbPrePass cbuffer = {};
    cbuffer.matView = pCurrentCameraState->matView;
    cbuffer.matInvView = pCurrentCameraState->matInvView;
    cbuffer.matProj = pCurrentCameraState->matProj;
    cbuffer.matInvProj = pCurrentCameraState->matInvProj;
    cbuffer.matViewProj = pCurrentCameraState->matViewProj;
    cbuffer.matInvViewProj = pCurrentCameraState->matInvViewProj;

    cbuffer.matViewProjPrev = (pPreviousCameraState != nullptr) ? pPreviousCameraState->matViewProj : cbuffer.matViewProj;
    cbuffer.matJitterViewProjPrev = (pPreviousCameraState != nullptr) ? pPreviousCameraState->matJitterViewProj : cbuffer.matJitterViewProj;

    cbuffer.matJitterViewProj = pCurrentCameraState->matJitterViewProj;
    cbuffer.matInvJitterViewProj = pCurrentCameraState->matInvJitterViewProj;

    cbuffer.nearClip = pCurrentCameraState->zNear;
    cbuffer.farClip = pCurrentCameraState->zFar;

    if (RenderSetting::Get().GetReversedZ()) {
        cbuffer.zBufferParams.x = -1.0 + cbuffer.farClip / cbuffer.nearClip;
        cbuffer.zBufferParams.y = 1.f;
    } else {
	    cbuffer.zBufferParams.x = 1.0 - cbuffer.farClip / cbuffer.nearClip;
        cbuffer.zBufferParams.y = cbuffer.farClip / cbuffer.nearClip;
    }
    cbuffer.zBufferParams.z = cbuffer.zBufferParams.x / cbuffer.farClip;
    cbuffer.zBufferParams.w = cbuffer.zBufferParams.y / cbuffer.farClip;

    glm::vec3 right;
    glm::vec3 up;
    glm::vec3 forward;
    glm::quat rotation = glm::quat_cast(cbuffer.matView);
    glm::Quaternion2BasisAxis(rotation, right, up, forward);

    glm::vec3 cameraPos = -glm::vec3(cbuffer.matView[3][0], cbuffer.matView[3][1], cbuffer.matView[3][2]);
    cbuffer.cameraPos = cameraPos;
    cbuffer.cameraLookUp = up;
    cbuffer.cameraLookAt = forward;
    cbuffer.renderTargetSize = {pCurrentCameraState->screenWidth, pCurrentCameraState->screenHeight};
    cbuffer.invRenderTargetSize = 1.f / cbuffer.renderTargetSize;

    cbuffer.totalTime = GameTimer::Get().GetTotalTimeS();
    cbuffer.deltaTime = GameTimer::Get().GetDeltaTimeS();
    return cbuffer;
}

auto MakeCbLighting(const SceneLightManager *pSceneLightManager) -> CbLighting {
	const std::vector<GameObject*> &directionalLightObjects = pSceneLightManager->GetDirectionalLightObjects();
	CbLighting cbuffer = {};

    // initialize directional light
    // If there is no directional light, the scene is black
    if (directionalLightObjects.size() > 0) {
	    GameObject *pGameObject = directionalLightObjects.back();

	    ::DirectionalLight *pDirectionalLight = pGameObject->GetComponent<::DirectionalLight>();
        cbuffer.directionalLight.color = pDirectionalLight->GetColor();
        cbuffer.directionalLight.intensity = pDirectionalLight->GetIntensity();

        glm::quat worldRotation = pGameObject->GetTransform()->GetWorldRotation();
        glm::vec3 x, y, z;
        Quaternion2BasisAxis(worldRotation, x, y, z);
        cbuffer.directionalLight.direction = -z;
    }

    // initialize ambient lighting
	RenderSetting &renderSetting = RenderSetting::Get();
    cbuffer.ambientLight.color = renderSetting.GetAmbientColor();
    cbuffer.ambientLight.intensity = renderSetting.GetAmbientIntensity();
    return cbuffer;
}

}    // namespace cbuffer
