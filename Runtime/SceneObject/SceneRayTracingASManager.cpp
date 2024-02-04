#include "SceneRayTracingASManager.h"
#include "Components/MeshRenderer.h"
#include "Components/Transform.h"
#include "D3d12/AccelerationStructure.h"
#include "D3d12/ASBuilder.h"
#include "D3d12/Device.h"
#include "D3d12/SwapChain.h"
#include "D3d12/TopLevelASGenerator.h"
#include "Foundation/CompileEnvInfo.hpp"
#include "Renderer/GfxDevice.h"

#include "Renderer/RenderUtils/FrameCaptrue.h"
#include "Foundation/DebugBreak.h"
#include "Object/GameObject.h"
#include "RenderObject/Material.h"
#include "RenderObject/Mesh.h"
#include "RenderObject/RenderGroup.hpp"

SceneRayTracingASManager::SceneRayTracingASManager() : _rebuildTopLevelAS(false) {
    _pAsyncASBuilder = std::make_unique<dx::AsyncASBuilder>();
    _pAsyncASBuilder->OnCreate(GfxDevice::GetInstance()->GetDevice());
}

SceneRayTracingASManager::~SceneRayTracingASManager() {
    _pAsyncASBuilder->OnDestroy();
}

void SceneRayTracingASManager::AddMeshRenderer(MeshRenderer *pMeshRenderer) {
    _meshRendererList.push_back(pMeshRenderer);
    _rebuildTopLevelAS = true;
}

void SceneRayTracingASManager::RemoveMeshRenderer(MeshRenderer *pMeshRenderer) {
    std::erase(_meshRendererList, pMeshRenderer);
    _rebuildTopLevelAS = true;
}

void SceneRayTracingASManager::RebuildTopLevelAS() {
    static bool sDebugBuildAS = false;
    if (CompileEnvInfo::IsModeDebug() && sDebugBuildAS) {
        GfxDevice *pGfxDevice = GfxDevice::GetInstance();
        HWND hwnd = pGfxDevice->GetSwapChain()->GetHWND();
        dx::Device *device = pGfxDevice->GetDevice();
        FrameCapture::BeginFrameCapture(hwnd, device);
    }

    _rayTracingGeometries.clear();
    std::vector<dx::ASInstance> instances;
    instances.reserve(_meshRendererList.size());
    for (MeshRenderer *pMeshRenderer : _meshRendererList) {
        const dx::ASInstance &instance = pMeshRenderer->GetASInstance();
        if (instance.IsValid()) {
            instances.push_back(instance);
            RayTracingGeometry geometry;
            geometry._pMaterial = pMeshRenderer->GetMaterial().get();
            geometry._pMesh = pMeshRenderer->GetMesh().get();
            geometry._instanceID = instance.instanceID;
            geometry._hitGroupIndex = instance.hitGroupIndex;
            geometry._instanceMask = instance.instanceMask;
            geometry._instanceFlag = instance.instanceFlag;
            geometry._transform = pMeshRenderer->GetGameObject()->GetTransform()->GetWorldMatrix();
            _rayTracingGeometries.push_back(geometry);
        }
    }

    dx::TopLevelASGenerator generator;
    generator.SetInstances(std::move(instances));
    if (_pTopLevelAs != nullptr && _pTopLevelAs->GetInstanceCount() == instances.size()) {
        _pTopLevelAs = generator.CommitBuildCommand(_pAsyncASBuilder.get(), _pTopLevelAs.Get());
    } else {
        _pTopLevelAs = generator.CommitBuildCommand(_pAsyncASBuilder.get());
    }

    _pAsyncASBuilder->Flush();
    dx::Device *pDevice = GfxDevice::GetInstance()->GetDevice();
    _pAsyncASBuilder->GetUploadFinishedFence().GpuWaitForFence(pDevice->GetGraphicsQueue());

    if (CompileEnvInfo::IsModeDebug() && sDebugBuildAS) {
        GfxDevice *pGfxDevice = GfxDevice::GetInstance();
        HWND hwnd = pGfxDevice->GetSwapChain()->GetHWND();
        dx::Device *device = pGfxDevice->GetDevice();
        _pAsyncASBuilder->GetUploadFinishedFence().CpuWaitForFence();
        FrameCapture::EndFrameCapture(hwnd, device);
        FrameCapture::OpenCaptureInUI();
        sDebugBuildAS = false;
        DEBUG_BREAK;
    }
}

void SceneRayTracingASManager::OnPreRender() {
    if (!_pAsyncASBuilder->IsIdle()) {
        _pAsyncASBuilder->GetUploadFinishedFence().CpuWaitForFence();
        _pAsyncASBuilder->Reset();
    }

    bool rebuildTopLevelAS = _rebuildTopLevelAS;
    for (MeshRenderer *pMeshRenderer : _meshRendererList) {
        if (pMeshRenderer->CheckASInstanceValidity(_pAsyncASBuilder.get())) {
            rebuildTopLevelAS = true;
        }
    }

    if (rebuildTopLevelAS) {
        RebuildTopLevelAS();
        _rebuildTopLevelAS = false;
    }
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

        RayTracingGeometry geometry;
        geometry._pMaterial = pMeshRenderer->GetMaterial().get();
        geometry._pMesh = pMeshRenderer->GetMesh().get();
        geometry._instanceID = pMeshRenderer->GetInstanceID();
        geometry._hitGroupIndex = hitGroupIndex++;
        geometry._instanceMask = 0xFF;    // todo
        geometry._instanceFlag = dx::RayTracingInstanceFlags::eNone;

        bool isTransparent = RenderGroup::IsTransparent(pMeshRenderer->GetMaterial()->GetRenderGroup());
        geometry._instanceFlag = SetOrClearFlags(geometry._instanceFlag,
            dx::RayTracingInstanceFlags::eForceOpaque,
            !isTransparent);
        geometry._instanceFlag = SetOrClearFlags(geometry._instanceFlag,
            dx::RayTracingInstanceFlags::eForceNonOpaque,
            isTransparent);

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

void SceneRayTracingASManager::BuildTopLevelAS(std::shared_ptr<RegionTopLevelAS> pRegionTopLevelAS) {

}

auto SceneRayTracingASManager::GetTopLevelAS() const -> dx::TopLevelAS * {
    return _pTopLevelAs.Get();
}

auto SceneRayTracingASManager::GetRayTracingGeometries() const -> const std::vector<RayTracingGeometry> & {
    return _rayTracingGeometries;
}
