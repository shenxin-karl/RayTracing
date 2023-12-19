#include "ConstantBufferHelper.h"
#include "Components/Camera.h"
#include "D3d12/Context.h"
#include "Object/GameObject.h"
#include "Components/Transform.h"
#include "Foundation/GameTimer.h"

namespace cbuffer {

auto MakePreObject(const Transform *pTransform) -> CbPreObject {
    CbPreObject cbuffer;
    cbuffer.matWorld = pTransform->GetWorldMatrix();
    cbuffer.matInvWorld = inverse(cbuffer.matWorld);
    cbuffer.matNormal = glm::WorldMatrixToNormalMatrix(cbuffer.matWorld);
    cbuffer.matInvNormal = inverse(cbuffer.matNormal);
    return cbuffer;
}

auto MakePrePass(const GameObject *pCameraGO) -> CbPrePass {
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

D3D12_GPU_VIRTUAL_ADDRESS AllocPreObjectCBuffer(dx::Context *pContext, const Transform *pTransform) {
	CbPreObject cbuffer = MakePreObject(pTransform);
    return pContext->AllocConstantBuffer(cbuffer);
}

D3D12_GPU_VIRTUAL_ADDRESS AllocPrePassCBuffer(dx::Context *pContext, const GameObject *pCameraGO) {
    CbPrePass cbuffer = MakePrePass(pCameraGO);
    return pContext->AllocConstantBuffer(cbuffer);
}

}    // namespace cbuffer
