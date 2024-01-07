#include "SceneRayTracingASManager.h"
#include "Components/MeshRenderer.h"
#include "D3d12/AccelerationStructure.h"
#include "D3d12/ASBuilder.h"
#include "D3d12/Device.h"
#include "D3d12/SwapChain.h"
#include "D3d12/TopLevelASGenerator.h"
#include "Foundation/CompileEnvInfo.hpp"
#include "Renderer/GfxDevice.h"

#include "Renderer/RenderUtils/FrameCaptrue.h"
#include "Foundation/DebugBreak.h"

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
    static bool sDebugBuildAS = 0;
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
            geometry.instanceID = instance.instanceID;
            geometry.hitGroupIndex = instance.hitGroupIndex;
            geometry.instanceMask = instance.instanceMask;
            geometry.pMesh = pMeshRenderer->GetMesh().get();
            geometry.pMaterial = pMeshRenderer->GetMaterial().get();
            _rayTracingGeometries.push_back(geometry);
        }
    }

    dx::TopLevelASGenerator generator;
    generator.SetInstances(std::move(instances));
    if (_pTopLevelAs != nullptr && _pTopLevelAs->GetInstanceCount() == instances.size()) {
        _pTopLevelAs = generator.CommitBuildCommand(_pAsyncASBuilder.get(), _pTopLevelAs.get());
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

auto SceneRayTracingASManager::GetTopLevelAS() const -> dx::TopLevelAS * {
    return _pTopLevelAs.get();
}

auto SceneRayTracingASManager::GetRayTracingGeometries() const -> const std::vector<RayTracingGeometry> & {
    return _rayTracingGeometries;
}
