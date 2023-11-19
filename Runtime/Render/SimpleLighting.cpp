#include "SimpleLighting.h"
#include "D3d12/ASBuilder.h"
#include "D3d12/BottomLevelASGenerator.h"
#include "D3d12/Context.h"
#include "D3d12/DescriptorManager.hpp"
#include "D3d12/Device.h"
#include "D3d12/FrameResource.h"
#include "D3d12/FrameResourceRing.h"
#include "D3d12/ShaderCompiler.h"
#include "D3d12/ShaderTableGenerator.h"
#include "D3d12/StaticBuffer.h"
#include "D3d12/SwapChain.h"
#include "D3d12/TopLevelASGenerator.h"
#include "Foundation/ColorUtil.hpp"
#include "Foundation/GameTimer.h"
#include "Foundation/Logger.h"
#include "InputSystem/InputSystem.h"
#include "InputSystem/Keyboard.h"
#include "Pix/Pix.h"
#include "ShaderLoader/ShaderManager.h"
#include "Utils/AssetProjectSetting.h"

static const wchar_t *sMyRayGenShader = L"MyRaygenShader";
static const wchar_t *sClosestHitShader = L"MyClosestHitShader";
static const wchar_t *sMissShader = L"MyMissShader";
static const wchar_t *sHitGroup = L"MyHitGroup";

namespace GlobalRootParams {
enum {
    Scene = 0,
    SceneCB = 1,
    Output = 2,
};
}

namespace LocalRootParams {

enum {
    CubeCB = 0,
    Indices = 1,
    Vertices = 2,
};

}

void SimpleLighting::OnCreate(uint32_t numBackBuffer, HWND hwnd) {
    Renderer::OnCreate(numBackBuffer, hwnd);
    SetupCamera();
    BuildGeometry();
    CreateRootSignature();
    CreateRayTracingPipeline();
    BuildAccelerationStructure();
}

void SimpleLighting::OnDestroy() {
    _pDevice->WaitForGPUFlush();
    _rayTracingOutput.OnDestroy();
    _rayTracingOutputHandle.Release();
    _pMeshBuffer->OnDestroy();
    _bottomLevelAs.OnDestroy();
    _topLevelAs.OnDestroy();
    _pASBuilder->OnDestroy();
    Renderer::OnDestroy();
}

void SimpleLighting::OnUpdate(GameTimer &timer) {
    Renderer::OnUpdate(timer);
    _pCamera->Update();
}

void SimpleLighting::OnPreRender(GameTimer &timer) {
    Renderer::OnPreRender(timer);

    const CameraData &cameraData = _pCamera->GetCameraData();
    _sceneConstantBuffer.projectToWorld = cameraData.matInvVewProj;
    _sceneConstantBuffer.cameraPosition = glm::vec4(cameraData.lookForm, 1.0f);
    _sceneConstantBuffer.lightPosition = glm::vec4(65.f, 45.f, 0.f, 0.f);
    _sceneConstantBuffer.lightAmbientColor = glm::vec4(0.1f);
    _sceneConstantBuffer.lightDiffuseColor = glm::vec4(0.9f);
}

void SimpleLighting::OnRender(GameTimer &timer) {

    static uint64_t frameCount = 0;
    if (static_cast<uint64_t>(timer.GetTotalTime()) > frameCount) {
        Logger::Info("fps {}", timer.GetFPS());
        frameCount = static_cast<uint64_t>(timer.GetTotalTime());
    }

    static bool waitCaptureFrameFinished = false;
    bool beginCapture = false;
    InputSystem *pInputSystem = InputSystem::GetInstance();
    if (!waitCaptureFrameFinished && pInputSystem->pKeyboard->IsKeyRelease(VK_F11)) {
        _pDevice->WaitForGPUFlush();
        Pix::BeginFrameCapture(_pSwapChain->GetHWND(), _pDevice.get());
        waitCaptureFrameFinished = true;
        beginCapture = true;
    }

    Renderer::OnRender(timer);

    _pFrameResourceRing->OnBeginFrame();
    dx::FrameResource &frameResource = _pFrameResourceRing->GetCurrentFrameResource();
    std::shared_ptr<dx::GraphicsContext> pGraphicsCtx = frameResource.AllocGraphicsContext();

    pGraphicsCtx->Transition(_rayTracingOutput.GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    pGraphicsCtx->SetComputeRootSignature(&_globalRootSignature);
    pGraphicsCtx->SetComputeRootShaderResourceView(GlobalRootParams::Scene, _topLevelAs.GetGPUVirtualAddress());
    pGraphicsCtx->SetComputeRootDynamicConstantBuffer(GlobalRootParams::SceneCB, _sceneConstantBuffer);
    pGraphicsCtx->SetDynamicViews(GlobalRootParams::Output, _rayTracingOutputHandle.GetCpuHandle());
    pGraphicsCtx->SetRayTracingPipelineState(_pRayTracingPSO.Get());

    D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = {};
    {
        dx::WRL::ComPtr<ID3D12StateObjectProperties> stateObjectProperties;
        dx::ThrowIfFailed(_pRayTracingPSO.As(&stateObjectProperties));
        void *pRayGenShaderIdentifier = stateObjectProperties->GetShaderIdentifier(sMyRayGenShader);
        void *pMissShaderIdentifier = stateObjectProperties->GetShaderIdentifier(sMissShader);
        void *pHitGroupShaderIdentifier = stateObjectProperties->GetShaderIdentifier(sHitGroup);

        dx::ShaderRecode rayGenShaderRecode(pRayGenShaderIdentifier);
        dispatchRaysDesc.RayGenerationShaderRecord = dx::MakeRayGenShaderRecode(pGraphicsCtx.get(), rayGenShaderRecode);

        dx::ShaderTableGenerator missShaderTable;
        missShaderTable.EmplaceShaderRecode(pMissShaderIdentifier);
        dispatchRaysDesc.MissShaderTable = missShaderTable.Generate(pGraphicsCtx.get());

        auto cubeCB = pGraphicsCtx->AllocConstantBuffer(Colors::Magenta);

        dx::ShaderTableGenerator hitGroupShaderTable;
        hitGroupShaderTable.EmplaceShaderRecode(pHitGroupShaderIdentifier,
            cubeCB,                             // gCubeCB
            _indexBufferView.BufferLocation,    // gIndices
            _vertexBufferView.BufferLocation);  // gVertices
        dispatchRaysDesc.HitGroupTable = hitGroupShaderTable.Generate(pGraphicsCtx.get());

        dispatchRaysDesc.Width = _width;
        dispatchRaysDesc.Height = _height;
        dispatchRaysDesc.Depth = 1;
    }
    pGraphicsCtx->DispatchRays(dispatchRaysDesc);
    pGraphicsCtx->Transition(_rayTracingOutput.GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE);
    pGraphicsCtx->Transition(_pSwapChain->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_COPY_DEST);
    pGraphicsCtx->CopyResource(_pSwapChain->GetCurrentBackBuffer(), _rayTracingOutput.GetResource());
    pGraphicsCtx->Transition(_pSwapChain->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT);
    frameResource.ExecuteContexts(pGraphicsCtx.get());

    _pSwapChain->Present();
    _pFrameResourceRing->OnEndFrame();


    if (beginCapture) {
        Pix::EndFrameCapture(_pSwapChain->GetHWND(), _pDevice.get());
        _pDevice->WaitForGPUFlush();
        float totalTime = timer.GetTotalTime();
        MainThread::AddMainThreadJob(MainThread::PreUpdate, [=](GameTimer &gameTimer) -> MainThread::JobStatus {
            if (gameTimer.GetTotalTime() >= totalTime + 5.f) {
                waitCaptureFrameFinished = false;
                Pix::OpenCaptureInUI();
                return MainThread::Finished;
            }
            return MainThread::ExecuteInNextFrame;
        });
    }
}

void SimpleLighting::OnResize(uint32_t width, uint32_t height) {
    Renderer::OnResize(width, height);
    CreateRayTracingOutput();
    _pCamera->SetAspect(static_cast<float>(width) / static_cast<float>(height));
}

void SimpleLighting::SetupCamera() {
    CameraDesc cameraDesc;
    cameraDesc.lookFrom = glm::vec3(5, 0, 0);
    cameraDesc.lookUp = glm::vec3(0, 1, 0);
    cameraDesc.lookAt = glm::vec3(0);
    cameraDesc.fov = 45.f;
    cameraDesc.nearClip = 0.1f;
    cameraDesc.farClip = 100.f;
    cameraDesc.aspect = static_cast<float>(_width) / static_cast<float>(_height);
    _pCamera = std::make_unique<Camera>(cameraDesc);
}

void SimpleLighting::BuildGeometry() {
    // clang-format off
	uint16_t indices[] = {
        3,1,0,
        2,1,3,

        6,4,5,
        7,4,6,

        11,9,8,
        10,9,11,

        14,12,13,
        15,12,14,

        19,17,16,
        18,17,19,

        22,20,21,
        23,20,22
    };
    Vertex vertices[] = {
        { glm::vec3(-1.0f, 1.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f) },
        { glm::vec3(1.0f, 1.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f) },
        { glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f) },
        { glm::vec3(-1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f) },

        { glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f) },
        { glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f) },
        { glm::vec3(1.0f, -1.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f) },
        { glm::vec3(-1.0f, -1.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f) },

        { glm::vec3(-1.0f, -1.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f) },
        { glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(-1.0f, 0.0f, 0.0f) },
        { glm::vec3(-1.0f, 1.0f, -1.0f), glm::vec3(-1.0f, 0.0f, 0.0f) },
        { glm::vec3(-1.0f, 1.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f) },

        { glm::vec3(1.0f, -1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f) },
        { glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(1.0f, 0.0f, 0.0f) },
        { glm::vec3(1.0f, 1.0f, -1.0f), glm::vec3(1.0f, 0.0f, 0.0f) },
        { glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f) },

        { glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(0.0f, 0.0f, -1.0f) },
        { glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(0.0f, 0.0f, -1.0f) },
        { glm::vec3(1.0f, 1.0f, -1.0f), glm::vec3(0.0f, 0.0f, -1.0f) },
        { glm::vec3(-1.0f, 1.0f, -1.0f), glm::vec3(0.0f, 0.0f, -1.0f) },

        { glm::vec3(-1.0f, -1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f) },
        { glm::vec3(1.0f, -1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f) },
        { glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f) },
        { glm::vec3(-1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f) },
    };
    // clang-format on

    constexpr size_t bufferSize = sizeof(indices) + sizeof(vertices);
    _pMeshBuffer = std::make_unique<dx::StaticBuffer>();
    _pMeshBuffer->OnCreate(_pDevice.get(), bufferSize);
    _pMeshBuffer->SetName("CubeMesh");

    dx::StaticBufferUploadHeap uploadHeap(*_pUploadHeap, *_pMeshBuffer);
    _vertexBufferView = uploadHeap.AllocVertexBuffer(std::size(vertices), sizeof(Vertex), vertices).value();
    _indexBufferView = uploadHeap.AllocIndexBuffer(std::size(indices), sizeof(uint16_t), indices).value();
    uploadHeap.DoUpload();
}

void SimpleLighting::CreateRayTracingOutput() {
    _rayTracingOutput.OnDestroy();

    DXGI_FORMAT backBufferFormat = _pSwapChain->GetFormat();
    CD3DX12_RESOURCE_DESC uavDesc = CD3DX12_RESOURCE_DESC::Tex2D(backBufferFormat,
        _width,
        _height,
        1,
        1,
        1,
        0,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    _rayTracingOutput.OnCreate(_pDevice.get(), uavDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr);
    _rayTracingOutput.SetName("RayTracingOutput");

    if (_rayTracingOutputHandle.IsNull()) {
        _rayTracingOutputHandle = dx::DescriptorManager::Alloc<dx::UAV>();
    }
    dx::NativeDevice *device = _pDevice->GetNativeDevice();
    D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc = {};
    viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    device->CreateUnorderedAccessView(_rayTracingOutput.GetResource(),
        nullptr,
        &viewDesc,
        _rayTracingOutputHandle.GetCpuHandle());
}

void SimpleLighting::CreateRootSignature() {
    // clang-format off
    _closestLocalRootSignature.Reset(3);
    {
        _closestLocalRootSignature.At(LocalRootParams::CubeCB).InitAsBufferCBV(1);                      // gCubeCB(b1)
        _closestLocalRootSignature.At(LocalRootParams::Indices).InitAsBufferSRV(1);                     // gIndices(t1)
        _closestLocalRootSignature.At(LocalRootParams::Vertices).InitAsBufferSRV(2);                    // gVertices(t2)
    }
    _closestLocalRootSignature.Finalize(_pDevice.get(), D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);

    _globalRootSignature.Reset(3);
    {
        _globalRootSignature.At(GlobalRootParams::Scene).InitAsBufferSRV(0);            // gScene(t0)
        _globalRootSignature.At(GlobalRootParams::SceneCB).InitAsBufferCBV(0);          // gSceneCB(b0)
        _globalRootSignature.At(GlobalRootParams::Output).InitAsDescriptorTable({ 
            CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0) // gOutput(u0);
        });          
    }
    _globalRootSignature.Finalize(_pDevice.get());
    // clang-format on
}

void SimpleLighting::CreateRayTracingPipeline() {
    ShaderManager *pShaderManager = ShaderManager::GetInstance();
    ShaderLoadInfo shaderLoadInfo;
    shaderLoadInfo.sourcePath = AssetProjectSetting::ToAssetPath("Shaders/SimpleLighting.hlsl");
    shaderLoadInfo.shaderType = dx::ShaderType::eLib;
    D3D12_SHADER_BYTECODE byteCode = pShaderManager->LoadShaderByteCode(shaderLoadInfo);
    Assert(byteCode.pShaderBytecode != nullptr);

    CD3DX12_STATE_OBJECT_DESC rayTracingPipeline{D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE};
    CD3DX12_DXIL_LIBRARY_SUBOBJECT *pLib = rayTracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
    pLib->SetDXILLibrary(&byteCode);
    pLib->DefineExport(sMyRayGenShader);
    pLib->DefineExport(sClosestHitShader);
    pLib->DefineExport(sMissShader);

    CD3DX12_HIT_GROUP_SUBOBJECT *pHitGroup = rayTracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
    pHitGroup->SetClosestHitShaderImport(sClosestHitShader);
    pHitGroup->SetHitGroupExport(sHitGroup);
    pHitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

    auto *pShaderConfig = rayTracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
    size_t payloadSize = 4 * sizeof(float);    // float4
    size_t attributeSize = 2 * sizeof(float);
    pShaderConfig->Config(payloadSize, attributeSize);

    CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT
    *pLocalRootSignature = rayTracingPipeline.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
    pLocalRootSignature->SetRootSignature(_closestLocalRootSignature.GetRootSignature());
    CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT *rootSignatureAssociation = rayTracingPipeline.CreateSubobject<
        CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
    rootSignatureAssociation->SetSubobjectToAssociate(*pLocalRootSignature);
    rootSignatureAssociation->AddExport(sClosestHitShader);

    auto *pGlobalRootSignature = rayTracingPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
    pGlobalRootSignature->SetRootSignature(_globalRootSignature.GetRootSignature());

    auto *pPipelineConfig = rayTracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
    pPipelineConfig->Config(1);

    dx::NativeDevice *device = _pDevice->GetNativeDevice();
    dx::ThrowIfFailed(device->CreateStateObject(rayTracingPipeline, IID_PPV_ARGS(&_pRayTracingPSO)));
}

void SimpleLighting::BuildAccelerationStructure() {
    _pASBuilder = std::make_unique<dx::ASBuilder>();
    _pASBuilder->OnCreate(_pDevice.get());
    _pASBuilder->BeginBuild();
    {
        dx::BottomLevelASGenerator bottomLevelAsGenerator;
        bottomLevelAsGenerator.AddGeometry(_vertexBufferView, DXGI_FORMAT_R32G32B32_FLOAT, _indexBufferView);
        _bottomLevelAs = bottomLevelAsGenerator.Generate(_pASBuilder.get());

        dx::TopLevelASGenerator topLevelAsGenerator;
        topLevelAsGenerator.AddInstance(_bottomLevelAs.GetResource(), glm::mat4x4(1.0), 0, 0);
        _topLevelAs = topLevelAsGenerator.Generate(_pASBuilder.get());
    }
    _pASBuilder->EndBuild();
    _pASBuilder->GetBuildFinishedFence().CpuWaitForFence();
}
