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

auto MakeCbPreObject(const Transform *pTransform) -> CbPreObject {
    CbPreObject cbuffer;
    cbuffer.matWorld = pTransform->GetWorldMatrix();
    cbuffer.matInvWorld = inverse(cbuffer.matWorld);
    cbuffer.matNormal = glm::WorldMatrixToNormalMatrix(cbuffer.matWorld);
    cbuffer.matInvNormal = inverse(cbuffer.matNormal);
    return cbuffer;
}

auto MakeCbPrePass(const GameObject *pCameraGO) -> CbPrePass {
    if (pCameraGO->GetComponent<Camera>() == nullptr) {
        Exception::Throw("the pCameraGo not Camera Component!");
    }

    const Camera *pCamera = pCameraGO->GetComponent<Camera>();
    const Transform *pTransform = pCameraGO->GetTransform();
    glm::quat rotation = pTransform->GetWorldRotation();

    CbPrePass cbuffer;
    cbuffer.matView = pCamera->GetViewMatrix();
    cbuffer.matInvView = pCamera->GetInverseViewMatrix();
    cbuffer.matProj = pCamera->GetProjectionMatrix();
    cbuffer.matInvProj = pCamera->GetInverseProjectionMatrix();
    cbuffer.matViewProj = pCamera->GetViewProjectionMatrix();
    cbuffer.matInvViewProj = pCamera->GetInverseViewProjectionMatrix();
    cbuffer.nearClip = pCamera->GetNearClip();
    cbuffer.farClip = pCamera->GetFarClip();

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
    glm::Quaternion2BasisAxis(rotation, right, up, forward);

    cbuffer.cameraPos = pTransform->GetWorldPosition();
    cbuffer.cameraLookUp = up;
    cbuffer.cameraLookAt = forward;
    cbuffer.renderTargetSize = {pCamera->GetScreenWidth(), pCamera->GetScreenHeight()};
    cbuffer.invRenderTargetSize = 1.f / cbuffer.renderTargetSize;

    cbuffer.totalTime = GameTimer::Get().GetTotalTime();
    cbuffer.deltaTime = GameTimer::Get().GetDeltaTime();
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

D3D12_GPU_VIRTUAL_ADDRESS AllocPreObjectCBuffer(dx::Context *pContext, const Transform *pTransform) {
	CbPreObject cbuffer = MakeCbPreObject(pTransform);
    return pContext->AllocConstantBuffer(cbuffer);
}

D3D12_GPU_VIRTUAL_ADDRESS AllocPrePassCBuffer(dx::Context *pContext, const GameObject *pCameraGO) {
    CbPrePass cbuffer = MakeCbPrePass(pCameraGO);
    return pContext->AllocConstantBuffer(cbuffer);
}

}    // namespace cbuffer
