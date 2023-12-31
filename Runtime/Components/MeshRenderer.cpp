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
    _pCurrentScene->GetRayTracingASManager()->RemoveMeshRenderer(this);
    _pCurrentScene = nullptr;
}

void MeshRenderer::OnAddToScene() {
    Component::OnAddToScene();
    SceneID sceneId = GetGameObject()->GetSceneID();
    _pCurrentScene = SceneManager::GetInstance()->GetScene(sceneId);
    _pCurrentScene->GetRayTracingASManager()->AddMeshRenderer(this);
    _instanceData.instanceID = GetInstanceID();
}

void MeshRenderer::OnPreRender() {
    Component::OnPreRender();
    CommitRenderObject();
}

bool MeshRenderer::CheckASInstanceValidity(dx::AsyncASBuilder *pAsyncAsBuilder) {
    bool transformDirty = GetGameObject()->GetTransform()->ThisFrameChanged();
    dx::BottomLevelAS *pBottomLevelAs = nullptr;
    if (_pMesh != nullptr) {
        if (_pMaterial != nullptr) {
            uint16_t renderGroup = _pMaterial->GetRenderGroup();
            if (RenderGroup::IsOpaque(renderGroup) || RenderGroup::IsAlphaTest(renderGroup)) {
                pBottomLevelAs = _pMesh->RequireBottomLevelAS(pAsyncAsBuilder, true);
            } else if (RenderGroup::IsTransparent(renderGroup)) {
                pBottomLevelAs = _pMesh->RequireBottomLevelAS(pAsyncAsBuilder, false);
            }
        }
    }

    ID3D12Resource *pBottomLevelASResource = nullptr;
    if (pBottomLevelAs != nullptr) {
        pBottomLevelASResource = pBottomLevelAs->GetResource();
    }

    if (transformDirty) {
        _instanceData.transform = GetGameObject()->GetTransform()->GetWorldMatrix();
    }

    bool needRebuild = transformDirty;
    needRebuild |= _instanceData.pBottomLevelAs != pBottomLevelASResource;
    _instanceData.pBottomLevelAs = pBottomLevelASResource;
    return needRebuild;
}

void MeshRenderer::CommitRenderObject() {
    if (_pMesh == nullptr || _pMaterial == nullptr) {
        return;
    }
    if (_pMesh->GetSemanticMask() != _renderData.meshSemanticMask ||
        _pMaterial.get() != _renderData.renderObject.pMaterial || _pMaterial->PipelineIDDirty()) {
        _renderData.meshSemanticMask = _pMesh->GetSemanticMask();
        _renderData.shouldRender = _pMaterial->UpdatePipelineID(_renderData.meshSemanticMask);
        _renderData.renderObject.pMaterial = _pMaterial.get();
        _renderData.renderObject.pMesh = _pMesh.get();
        _renderData.renderObject.pTransform = GetGameObject()->GetTransform();
    }
    if (_renderData.shouldRender) {
        SceneRenderObjectManager *pRenderObjectMgr = _pCurrentScene->GetRenderObjectManager();
        pRenderObjectMgr->AddRenderObject(&_renderData.renderObject);
    }
}
