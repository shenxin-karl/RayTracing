#include "MeshRenderer.h"
#include "Object/GameObject.h"
#include "RenderObject/Mesh.h"
#include "SceneObject/SceneManager.h"
#include "RenderObject/StandardMaterial.h"
#include "RenderObject/StandardMaterial/StandardMaterial.h"
#include "SceneObject/Scene.h"
#include "SceneObject/SceneRenderObjectManager.h"

MeshRenderer::MeshRenderer() : _renderData{} {
    SetTickType(ePreRender);
}

void MeshRenderer::SetMesh(std::shared_ptr<Mesh> pMesh) {
    _pMesh = std::move(pMesh);
}

void MeshRenderer::SetMaterial(std::shared_ptr<StandardMaterial> pMaterial) {
    _pMaterial = std::move(pMaterial);
}

void MeshRenderer::OnRemoveFormScene() {
    Component::OnRemoveFormScene();
    _renderData.pCurrentScene = nullptr;
}

void MeshRenderer::OnAddToScene() {
    Component::OnAddToScene();
    SceneID sceneId = GetGameObject()->GetSceneID();
    _renderData.pCurrentScene = SceneManager::GetInstance()->GetScene(sceneId);
}

void MeshRenderer::OnPreRender() {
    Component::OnPreRender();
    if (_pMesh == nullptr || _pMaterial == nullptr) {
        return;
    }

    if (_pMesh->GetSemanticMask() != _renderData.meshSemanticMask ||
        _pMaterial.get() != _renderData.renderObject.pMaterial || _pMaterial->PipelineStateDirty()) {
        _renderData.meshSemanticMask = _pMesh->GetSemanticMask();
        _renderData.shouldRender = _pMaterial->UpdatePipelineState(_renderData.meshSemanticMask);
        _renderData.renderObject.pMaterial = _pMaterial.get();
        _renderData.renderObject.pMesh = _pMesh.get();
        _renderData.renderObject.pTransform = GetGameObject()->GetTransform();
    }

    if (_renderData.shouldRender) {
        SceneRenderObjectManager *pRenderObjectMgr = _renderData.pCurrentScene->GetRenderObjectManager();
        pRenderObjectMgr->AddRenderObject(&_renderData.renderObject);
    }
}
