#include "SimpleLighting.h"
#include "Components/Camera.h"
#include "Components/CameraColtroller.h"
#include "Components/Transform.h"
#include "D3d12/ASBuilder.h"
#include "D3d12/BottomLevelASGenerator.h"
#include "D3d12/Context.h"
#include "D3d12/Device.h"
#include "D3d12/FrameResource.h"
#include "D3d12/FrameResourceRing.h"
#include "D3d12/ShaderCompiler.h"
#include "D3d12/Buffer.h"
#include "D3d12/SwapChain.h"
#include "D3d12/TopLevelASGenerator.h"
#include "D3d12/UploadHeap.h"
#include "Foundation/ColorUtil.hpp"
#include "Foundation/GameTimer.h"
#include "Foundation/Logger.h"
#include "InputSystem/InputSystem.h"
#include "InputSystem/Keyboard.h"
#include "Object/GameObject.h"
#include "Renderer/GUI/GUI.h"
#include "Renderer/RenderUtils/FrameCaptrue.h"
#include "SceneObject/Scene.h"
#include "SceneObject/SceneManager.h"
#include "ShaderLoader/ShaderManager.h"
#include "TextureObject/TextureLoader.h"
#include "Utils/AssetProjectSetting.h"

static const wchar_t *sMyRayGenShader = L"MyRaygenShader";
static const wchar_t *sClosestHitShader = L"MyClosestHitShader";
static const wchar_t *sMissShader = L"MyMissShader";
static const wchar_t *sHitGroup = L"MyHitGroup";

namespace GlobalRootParams {
enum {
    Scene = 0,
    SceneCB = 1,
    Table0 = 2,
};

enum Table0Offset {
    Output = 0,
    CubeMap = 1,
};

}    // namespace GlobalRootParams

namespace LocalRootParams {

enum {
    CubeCB = 0,
    Indices = 1,
    Vertices = 2,
};

}

SimpleLighting::SimpleLighting() {
}

SimpleLighting::~SimpleLighting() {
}

void SimpleLighting::OnCreate() {
    Renderer::OnCreate();
    BuildGeometry();
    CreateRootSignature();
    CreateRayTracingPipeline();
    BuildAccelerationStructure();
    LoadCubeMap();
    InitScene();
}

void SimpleLighting::OnDestroy() {
    _pDevice->WaitForGPUFlush();
    _rayTracingOutput.Release();
    _rayTracingOutputHandle.Release();
    _pMeshBuffer.Release();
    _pBottomLevelAs.Release();
    _pTopLevelAs.Release();
    _pASBuilder->OnDestroy();
    Renderer::OnDestroy();
}

void SimpleLighting::OnUpdate(GameTimer &timer) {
    Renderer::OnUpdate(timer);
    ShowFPS();
}

void SimpleLighting::OnPreRender(GameTimer &timer) {
    Renderer::OnPreRender(timer);

    Camera *pCamera = _pGameObject->GetComponent<Camera>();
    Transform *pTransform = _pGameObject->GetTransform();
    _sceneConstantBuffer.projectToWorld = pCamera->GetInverseViewProjectionMatrix();
    _sceneConstantBuffer.cameraPosition = glm::vec4(pTransform->GetWorldPosition(), 1.0f);
    _sceneConstantBuffer.lightPosition = glm::vec4(65.f, 45.f, 0.f, 0.f);
    _sceneConstantBuffer.lightAmbientColor = glm::vec4(0.1f);
    _sceneConstantBuffer.lightDiffuseColor = glm::vec4(0.9f);
    _sceneConstantBuffer.time = timer.GetTotalTimeS();
}

void SimpleLighting::OnRender(GameTimer &timer) {
    bool beginCapture = InputSystem::GetInstance()->pKeyboard->IsKeyClicked(VK_F11);
    if (beginCapture) {
        FrameCapture::BeginFrameCapture(_pSwapChain->GetHWND(), _pDevice);
    }

    Renderer::OnRender(timer);

    _pFrameResourceRing->OnBeginFrame();
    dx::FrameResource &frameResource = _pFrameResourceRing->GetCurrentFrameResource();
    std::shared_ptr<dx::GraphicsContext> pGraphicsCtx = frameResource.AllocGraphicsContext();

    pGraphicsCtx->Transition(_rayTracingOutput->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    pGraphicsCtx->SetComputeRootSignature(_pGlobalRootSignature.Get());
    pGraphicsCtx->SetComputeRootShaderResourceView(GlobalRootParams::Scene, _pTopLevelAs->GetGPUVirtualAddress());
    pGraphicsCtx->SetComputeRootDynamicConstantBuffer(GlobalRootParams::SceneCB, _sceneConstantBuffer);
    pGraphicsCtx->SetDynamicViews(GlobalRootParams::Table0,
        _rayTracingOutputHandle.GetCpuHandle(),
        GlobalRootParams::Table0Offset::Output);
    pGraphicsCtx->SetDynamicViews(GlobalRootParams::Table0,
        _cubeMapHandle.GetCpuHandle(),
        GlobalRootParams::Table0Offset::CubeMap);
    pGraphicsCtx->SetRayTracingPipelineState(_pRayTracingPSO.Get());

    {
        dx::WRL::ComPtr<ID3D12StateObjectProperties> stateObjectProperties;
        dx::ThrowIfFailed(_pRayTracingPSO.As(&stateObjectProperties));
        void *pRayGenShaderIdentifier = stateObjectProperties->GetShaderIdentifier(sMyRayGenShader);
        void *pMissShaderIdentifier = stateObjectProperties->GetShaderIdentifier(sMissShader);
        void *pHitGroupShaderIdentifier = stateObjectProperties->GetShaderIdentifier(sHitGroup);

        dx::DispatchRaysDesc dispatchRaysDesc = {};
        dispatchRaysDesc.rayGenerationShaderRecode.SetShaderIdentifier(pRayGenShaderIdentifier);
        dispatchRaysDesc.missShaderTable.push_back(dx::ShaderRecode(pMissShaderIdentifier));

        CubeConstantBuffer cbuffer;
        cbuffer.albedo = glm::vec4(std::sin(timer.GetTotalTimeS()) * 0.5 + 0.5f, std::cos(timer.GetTotalTimeS()) * 0.5 + 0.5f, 1.f, 1.f);
        cbuffer.noiseTile = (std::sin(timer.GetTotalTimeS() * 0.5f) * 0.5 + 0.5) + 2.f;
        auto cubeCB = pGraphicsCtx->AllocConstantBuffer(cbuffer);

        dx::ShaderRecode hitGroupShaderRecode(pHitGroupShaderIdentifier, _pClosestLocalRootSignature.Get());
        dx::LocalRootParameterData &localRootParameterData = hitGroupShaderRecode.GetLocalRootParameterData();
        localRootParameterData.SetView(LocalRootParams::CubeCB, cubeCB);
        localRootParameterData.SetView(LocalRootParams::Indices, _indexBufferView.BufferLocation);
        localRootParameterData.SetView(LocalRootParams::Vertices, _vertexBufferView.BufferLocation);
        dispatchRaysDesc.hitGroupTable.push_back(std::move(hitGroupShaderRecode));

        dispatchRaysDesc.width = _width;
        dispatchRaysDesc.height = _height;
        dispatchRaysDesc.depth = 1;
		pGraphicsCtx->DispatchRays(dispatchRaysDesc);
    }
    pGraphicsCtx->Transition(_rayTracingOutput->GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE);
    pGraphicsCtx->Transition(_pSwapChain->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_COPY_DEST);
    pGraphicsCtx->CopyResource(_pSwapChain->GetCurrentBackBuffer(), _rayTracingOutput->GetResource());
    pGraphicsCtx->Transition(_pSwapChain->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT);
    frameResource.ExecuteContexts(pGraphicsCtx.get());

    GUI::Get().Render();
    _pSwapChain->Present();

    if (beginCapture) {
        FrameCapture::EndFrameCapture(_pSwapChain->GetHWND(), _pDevice);
		FrameCapture::OpenCaptureInUI();
    }
}

void SimpleLighting::OnResize(uint32_t width, uint32_t height) {
    Renderer::OnResize(width, height);
    _pGameObject->GetComponent<Camera>()->SetAspect(width, height);
    CreateRayTracingOutput();
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
    _pMeshBuffer = dx::Buffer::CreateStatic(_pDevice, bufferSize);
    _pMeshBuffer->SetName("CubeMesh");

    dx::StaticBufferUploadHeap uploadHeap(_pUploadHeap, _pMeshBuffer.Get());
    _vertexBufferView = uploadHeap.AllocVertexBuffer(std::size(vertices), sizeof(Vertex), vertices).value();
    _indexBufferView = uploadHeap.AllocIndexBuffer(std::size(indices), sizeof(uint16_t), indices).value();
    uploadHeap.CommitUploadCommand();
}

void SimpleLighting::CreateRayTracingOutput() {
    DXGI_FORMAT backBufferFormat = _pSwapChain->GetFormat();
    CD3DX12_RESOURCE_DESC uavDesc = CD3DX12_RESOURCE_DESC::Tex2D(backBufferFormat,
        _width,
        _height,
        1,
        1,
        1,
        0,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    _rayTracingOutput = dx::Texture::Create(_pDevice, uavDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr);
    _rayTracingOutput->SetName("RayTracingOutput");

    if (_rayTracingOutputHandle.IsNull()) {
        _rayTracingOutputHandle = _pDevice->AllocDescriptor<dx::UAV>(1);
    }
    dx::NativeDevice *device = _pDevice->GetNativeDevice();
    D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc = {};
    viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    device->CreateUnorderedAccessView(_rayTracingOutput->GetResource(),
        nullptr,
        &viewDesc,
        _rayTracingOutputHandle.GetCpuHandle());
}

void SimpleLighting::CreateRootSignature() {
    // clang-format off
    _pClosestLocalRootSignature = dx::RootSignature::Create(3);
    {
        _pClosestLocalRootSignature->At(LocalRootParams::CubeCB).InitAsBufferCBV(1);                      // gCubeCB(b1)
        _pClosestLocalRootSignature->At(LocalRootParams::Indices).InitAsBufferSRV(1);                     // gIndices(t1)
        _pClosestLocalRootSignature->At(LocalRootParams::Vertices).InitAsBufferSRV(2);                    // gVertices(t2)
    }
    _pClosestLocalRootSignature->Generate(_pDevice, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);

    _pGlobalRootSignature = dx::RootSignature::Create(3, 1);
    {
        _pGlobalRootSignature->At(GlobalRootParams::Scene).InitAsBufferSRV(0);            // gScene(t0)
        _pGlobalRootSignature->At(GlobalRootParams::SceneCB).InitAsBufferCBV(0);          // gSceneCB(b0)
        _pGlobalRootSignature->At(GlobalRootParams::Table0).InitAsDescriptorTable({ 
            CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0), // gOutput(u0);
            CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3)  // gCubeMap(t3);
        });
    }
    _pGlobalRootSignature->SetStaticSamplers(dx::GetLinearClampStaticSampler(0));        // s0
    _pGlobalRootSignature->Generate(_pDevice);
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
    pLocalRootSignature->SetRootSignature(_pClosestLocalRootSignature->GetRootSignature());
    CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT *rootSignatureAssociation = rayTracingPipeline.CreateSubobject<
        CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
    rootSignatureAssociation->SetSubobjectToAssociate(*pLocalRootSignature);
    rootSignatureAssociation->AddExport(sClosestHitShader);

    auto *pGlobalRootSignature = rayTracingPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
    pGlobalRootSignature->SetRootSignature(_pGlobalRootSignature->GetRootSignature());

    auto *pPipelineConfig = rayTracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
    pPipelineConfig->Config(1);

    dx::NativeDevice *device = _pDevice->GetNativeDevice();
#if ENABLE_RAY_TRACING
    dx::ThrowIfFailed(device->CreateStateObject(rayTracingPipeline, IID_PPV_ARGS(&_pRayTracingPSO)));
#endif
}

void SimpleLighting::BuildAccelerationStructure() {
    dx::BottomLevelASGenerator bottomLevelAsGenerator;
    bottomLevelAsGenerator.AddGeometry(_vertexBufferView, DXGI_FORMAT_R32G32B32_FLOAT, _indexBufferView);
    _pBottomLevelAs = bottomLevelAsGenerator.CommitBuildCommand(_pASBuilder);

    dx::TopLevelASGenerator topLevelAsGenerator;
    topLevelAsGenerator.AddInstance(_pBottomLevelAs->GetResource(), glm::mat4x4(1.0), 0, 0);
    _pTopLevelAs = topLevelAsGenerator.CommitBuildCommand(_pASBuilder);
}

void SimpleLighting::LoadCubeMap() {
    stdfs::path path = AssetProjectSetting::ToAssetPath("Textures/snowcube1024.dds");

    TextureLoader textureLoader;
    _pCubeMap = textureLoader.LoadFromFile(path);
    _cubeMapHandle = textureLoader.GetSRVCube(_pCubeMap.Get());
}

void SimpleLighting::InitScene() {
    _pScene = SceneManager::GetInstance()->CreateScene("Scene");
    _pGameObject = GameObject::Create();
    _pGameObject->AddComponent<Camera>();
    _pGameObject->AddComponent<CameraController>();
    _pGameObject->GetTransform()->SetLocalPosition(glm::vec3(0, 5, 5));
    _pGameObject->GetTransform()->LookAt(glm::vec3(0, 0, -1));
    _pScene->AddGameObject(_pGameObject);
}
