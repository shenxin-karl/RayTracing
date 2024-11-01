#include "MeshRenderer.h"
#include "Transform.h"
#include "D3d12/AccelerationStructure.h"
#include "D3d12/ASBuilder.h"
#include "Object/GameObject.h"
#include "RenderObject/Mesh.h"
#include "SceneObject/SceneManager.h"
#include "RenderObject/Material.h"
#include "RenderObject/RenderGroup.hpp"
#include "SceneObject/Scene.h"
#include "SceneObject/SceneRayTracingASManager.h"
#include "SceneObject/SceneRenderObjectManager.h"

MeshRenderer::MeshRenderer() : _renderData{} {
    SetTickType(ePreRender);
}

void MeshRenderer::SetMesh(std::shared_ptr<Mesh> pMesh) {
    _pMesh = std::move(pMesh);
}

void MeshRenderer::SetMaterial(std::shared_ptr<Material> pMaterial) {
    _pMaterial = std::move(pMaterial);
}

void MeshRenderer::OnRemoveFormScene() {
    Component::OnRemoveFormScene();
#if ENABLE_RAY_TRACING
    _pCurrentScene->GetRayTracingASManager()->RemoveMeshRenderer(this);
#endif
    _pCurrentScene = nullptr;
}

void MeshRenderer::OnAddToScene() {
    Component::OnAddToScene();
    SceneID sceneId = GetGameObject()->GetSceneID();
    _pCurrentScene = SceneManager::GetInstance()->GetScene(sceneId);
#if ENABLE_RAY_TRACING
    _pCurrentScene->GetRayTracingASManager()->AddMeshRenderer(this);
#endif
    _instanceData.instanceID = GetInstanceID();
}

void MeshRenderer::OnAddToGameObject() {
    Component::OnAddToGameObject();
    _renderData.renderObject.pTransform = GetGameObject()->GetTransform();
    _renderData.renderObject.cbPreObject.matWorld = glm::identity<glm::mat4x4>();
}

void MeshRenderer::OnPreRender() {
    Component::OnPreRender();
    CommitRenderObject();
}

bool MeshRenderer::PrepareAccelerationStructure() const {
    if (_pMaterial == nullptr || _pMesh == nullptr) {
	    return false;
    }
    if (_pMesh->IsGpuDataDirty()) {
	    return false;
    }
    return _pMesh->GetVertexCount() > 0;
}

auto MeshRenderer::GetBottomLevelAS() const -> dx::BottomLevelAS * {
    return _pMesh != nullptr ? _pMesh->GetBottomLevelAS() : nullptr;
}

void MeshRenderer::CommitRenderObject() {
    if (_pMesh == nullptr || _pMaterial == nullptr) {
        return;
    }

    Transform *pTransform = GetGameObject()->GetTransform();
    bool updateRenderObject = _pMesh->GetSemanticMask() != _renderData.meshSemanticMask ||
                              _pMaterial.get() != _renderData.renderObject.pMaterial || _pMaterial->PipelineIDDirty();

    if (updateRenderObject) {
        _renderData.meshSemanticMask = _pMesh->GetSemanticMask();
        _renderData.shouldRender = _pMaterial->UpdatePipelineID(_renderData.meshSemanticMask);
        _renderData.renderObject.pMaterial = _pMaterial.get();
        _renderData.renderObject.pMesh = _pMesh.get();
    }

    cbuffer::CbPreObject &cbPreObject = _renderData.renderObject.cbPreObject;
    cbPreObject.matWorldPrev = cbPreObject.matWorld;
    if (pTransform->ThisFrameChanged()) {
        cbPreObject.matWorld = pTransform->GetWorldMatrix();
        cbPreObject.matInvWorld = inverse(cbPreObject.matWorld);
        cbPreObject.matNormal = glm::WorldMatrixToNormalMatrix(cbPreObject.matWorld);
    }

    if (_renderData.shouldRender) {
        SceneRenderObjectManager *pRenderObjectMgr = _pCurrentScene->GetRenderObjectManager();
        pRenderObjectMgr->AddRenderObject(&_renderData.renderObject);
    }
}
