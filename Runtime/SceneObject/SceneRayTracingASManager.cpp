#include "SceneRayTracingASManager.h"

#include "Components/MeshRenderer.h"
#include "Components/Transform.h"
#include "D3d12/AccelerationStructure.h"
#include "D3d12/ASBuilder.h"
#include "D3d12/Device.h"
#include "D3d12/SwapChain.h"
#include "D3d12/TopLevelASGenerator.h"
#include "Object/GameObject.h"
#include "Renderer/GfxDevice.h"
#include "RenderObject/Material.h"
#include "RenderObject/Mesh.h"
#include "RenderObject/RenderGroup.hpp"

SceneRayTracingASManager::SceneRayTracingASManager() {
    _pAsyncASBuilder = std::make_unique<dx::AsyncASBuilder>();
    _pAsyncASBuilder->OnCreate(GfxDevice::GetInstance()->GetDevice());
}

SceneRayTracingASManager::~SceneRayTracingASManager() {
    _pAsyncASBuilder->OnDestroy();
}

void SceneRayTracingASManager::AddMeshRenderer(MeshRenderer *pMeshRenderer) {
    _meshRendererList.push_back(pMeshRenderer);
}

void SceneRayTracingASManager::RemoveMeshRenderer(MeshRenderer *pMeshRenderer) {
    std::erase(_meshRendererList, pMeshRenderer);
}

void SceneRayTracingASManager::BeginBuildBottomLevelAS() {
    if (!_pAsyncASBuilder->IsIdle()) {
        _pAsyncASBuilder->GetUploadFinishedFence().CpuWaitForFence();
        _pAsyncASBuilder->Reset();
    }
}

auto SceneRayTracingASManager::BuildMeshBottomLevelAS() -> std::shared_ptr<RegionTopLevelAS> {
    std::shared_ptr<RegionTopLevelAS> pRegionTopLevelAS = RegionTopLevelAS::Create();

    size_t hitGroupIndex = 0;
    for (MeshRenderer *pMeshRenderer : _meshRendererList) {
        if (!pMeshRenderer->PrepareAccelerationStructure()) {
            continue;
        }

        bool isTransparent = RenderGroup::IsTransparent(pMeshRenderer->GetMaterial()->GetRenderGroup());
        RayTracingGeometry geometry;
        geometry._pMeshRenderer = pMeshRenderer;
        geometry._instanceID = pMeshRenderer->GetInstanceID();
        geometry._hitGroupIndex = hitGroupIndex++;
        geometry._instanceMask = 0xFF;    // todo
        geometry._instanceFlag = isTransparent ? dx::RayTracingInstanceFlags::eForceNonOpaque
                                               : dx::RayTracingInstanceFlags::eNone;
        geometry._transform = pMeshRenderer->GetGameObject()->GetTransform()->GetWorldMatrix();
        pRegionTopLevelAS->_geometries.push_back(geometry);

        pMeshRenderer->GetMesh()->RequireBottomLevelAS(_pAsyncASBuilder.get());
    }
    return pRegionTopLevelAS;
}

void SceneRayTracingASManager::EndBuildBottomLevelAS() {
    _pAsyncASBuilder->Flush();
    dx::Device *pDevice = GfxDevice::GetInstance()->GetDevice();
    _pAsyncASBuilder->GetUploadFinishedFence().GpuWaitForFence(pDevice->GetGraphicsQueue());
}

void SceneRayTracingASManager::BuildTopLevelAS(dx::ComputeContext *pComputeContext,
    const std::shared_ptr<RegionTopLevelAS> &pRegionTopLevelAS) {

    dx::TopLevelASGenerator generator;
    generator.Reserve(pRegionTopLevelAS->GetGeometries().size());
    for (RayTracingGeometry &geometry : pRegionTopLevelAS->GetGeometries()) {
        const MeshRenderer *pMeshRender = geometry.GetMeshRenderer();
        dx::BottomLevelAS *pBottomLevelAS = pMeshRender->GetBottomLevelAS();
        Assert(pBottomLevelAS != nullptr);
        generator.AddInstance(pBottomLevelAS->GetResource(),
            geometry.GetTransform(),
            geometry.GetInstanceID(),
            geometry.GetHitGroupIndex(),
            geometry.GetInstanceMask());
    }

    GfxDevice *pGfxDevice = GfxDevice::GetInstance();

    // TopLevelASGenerator allocates a new memory each time
    SharedPtr<dx::Buffer> pInstanceBuffer = nullptr;
    dx::TopLevelASGenerator::BuildArgs buildArgs = {
        pGfxDevice->GetDevice(),
        pComputeContext,
        pInstanceBuffer,
        _pScratchBuffer,
    };
    pRegionTopLevelAS->_pTopLevelAS = generator.Build(buildArgs);
}
