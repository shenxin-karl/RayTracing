#include "TriangleRenderer.h"
#include "RenderUtils/FrameCaptrue.h"
#include "D3d12/BottomLevelASGenerator.h"
#include "D3d12/Context.h"
#include "D3d12/Device.h"
#include "D3d12/FrameResource.h"
#include "D3d12/FrameResourceRing.h"
#include "D3d12/ShaderCompiler.h"
#include "D3d12/StaticBuffer.h"
#include "D3d12/SwapChain.h"
#include "D3d12/TopLevelASGenerator.h"
#include "D3d12/UploadHeap.h"
#include "Foundation/GameTimer.h"
#include "Foundation/Logger.h"
#include "InputSystem/InputSystem.h"
#include "InputSystem/Keyboard.h"
#include "RenderUtils/GUI.h"
#include "ShaderLoader/ShaderManager.h"
#include "Utils/AssetProjectSetting.h"

enum RootIndex {
    AccelerationStructureSlot = 0,
    OutputRenderTarget = 1,
};

static std::wstring_view RayGenShaderName = L"MyRayGenShader";
static std::wstring_view ClosestHitShaderName = L"MyClosestHitShader";
static std::wstring_view MissShaderName = L"MyMissShader";
static std::wstring_view HitGroupName = L"MyHitGroup";

void TriangleRenderer::OnCreate() {
    Renderer::OnCreate();
    CreateGeometry();
    CreateRootSignature();
    CreateRayTracingPipelineStateObject();
    BuildAccelerationStructures();
    _pUploadHeap->FlushAndFinish();
    _pASBuilder->FlushAndFinish();
    _rayGenConstantBuffer.viewport = {-1.0f, -1.0f, 1.0f, 1.0f};
}

void TriangleRenderer::OnDestroy() {
    _pTriangleStaticBuffer->OnDestroy();
    _pASBuilder->OnDestroy();
    _bottomLevelAs->OnDestroy();
    _topLevelAs->OnDestroy();
    Renderer::OnDestroy();
}

void TriangleRenderer::OnPreRender(GameTimer &timer) {
    Renderer::OnPreRender(timer);
}

void TriangleRenderer::OnRender(GameTimer &timer) {
    Renderer::OnRender(timer);

    static uint64_t frameCount = 0;
    if (static_cast<uint64_t>(timer.GetTotalTime()) > frameCount) {
        Logger::Info("fps {}", timer.GetFPS());
        frameCount = static_cast<uint64_t>(timer.GetTotalTime());
    }

    InputSystem *pInputSystem = InputSystem::GetInstance();
    bool beginCapture = pInputSystem->pKeyboard->IsKeyClicked(VK_F11);
    if (beginCapture) {
        FrameCapture::BeginFrameCapture(_pSwapChain->GetHWND(), _pDevice);
    }

    _pFrameResourceRing->OnBeginFrame();
    dx::FrameResource &frameResource = _pFrameResourceRing->GetCurrentFrameResource();
    std::shared_ptr<dx::GraphicsContext> pGraphicsCtx = frameResource.AllocGraphicsContext();

    pGraphicsCtx->Transition(_rayTracingOutput.GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    pGraphicsCtx->SetComputeRootSignature(&_globalRootSignature);
    pGraphicsCtx->SetComputeRootShaderResourceView(AccelerationStructureSlot, _topLevelAs->GetGPUVirtualAddress());
    pGraphicsCtx->SetDynamicViews(OutputRenderTarget, _rayTracingOutputView.GetCpuHandle());
    pGraphicsCtx->SetRayTracingPipelineState(_pRayTracingPSO.Get());

    // build dispatch rays desc
    {
        dx::WRL::ComPtr<ID3D12StateObjectProperties> stateObjectProperties;
        dx::ThrowIfFailed(_pRayTracingPSO.As(&stateObjectProperties));
        void *pRayGenShaderIdentifier = stateObjectProperties->GetShaderIdentifier(RayGenShaderName.data());
        void *pMissShaderIdentifier = stateObjectProperties->GetShaderIdentifier(MissShaderName.data());
        void *pHitGroupShaderIdentifier = stateObjectProperties->GetShaderIdentifier(HitGroupName.data());

        dx::DispatchRaysDesc dispatchRaysDesc = {};
        dx::ShaderRecode rayGenShaderRecode(pRayGenShaderIdentifier, &_localRootSignature);
        rayGenShaderRecode.GetLocalRootParameterData().SetConstants(0,
            dx::SizeofInUint32<RayGenConstantBuffer>(),
            &_rayGenConstantBuffer);

        dispatchRaysDesc.rayGenerationShaderRecode = std::move(rayGenShaderRecode);
        dispatchRaysDesc.missShaderTable.push_back(dx::ShaderRecode(pMissShaderIdentifier));
        dispatchRaysDesc.hitGroupTable.push_back(dx::ShaderRecode(pHitGroupShaderIdentifier));
        dispatchRaysDesc.width = _width;
        dispatchRaysDesc.height = _height;
        dispatchRaysDesc.depth = 1;
        pGraphicsCtx->DispatchRays(dispatchRaysDesc);
    }

    pGraphicsCtx->Transition(_rayTracingOutput.GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE);
    pGraphicsCtx->Transition(_pSwapChain->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_COPY_DEST);
    pGraphicsCtx->CopyResource(_pSwapChain->GetCurrentBackBuffer(), _rayTracingOutput.GetResource());

    pGraphicsCtx->Transition(_pSwapChain->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT);
    frameResource.ExecuteContexts(pGraphicsCtx.get());

    GUI::Get().Render();
    _pSwapChain->Present();
    _pFrameResourceRing->OnEndFrame();

    if (beginCapture) {
        FrameCapture::EndFrameCapture(_pSwapChain->GetHWND(), _pDevice);
        FrameCapture::OpenCaptureInUI();
    }
}

void TriangleRenderer::OnResize(uint32_t width, uint32_t height) {
    Renderer::OnResize(width, height);
    CreateRayTracingOutputResource();

    float border = 0.1f;
    float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    if (width <= height) {
        _rayGenConstantBuffer.stencil = {-1 + border,
            -1 + border * aspectRatio,
            1.0f - border,
            1 - border * aspectRatio};
    } else {
        _rayGenConstantBuffer.stencil = {-1 + border / aspectRatio,
            -1 + border,
            1 - border / aspectRatio,
            1.0f - border};
    }
}

void TriangleRenderer::CreateGeometry() {
    _pTriangleStaticBuffer = std::make_shared<dx::StaticBuffer>();
    uint16_t indices[] = {0, 1, 2};

    float depthValue = 1.0;
    float offset = 0.7f;
    glm::vec3 vertices[] = {{0, -offset, depthValue}, {-offset, offset, depthValue}, {offset, offset, depthValue}};

    _pTriangleStaticBuffer->OnCreate(_pDevice, sizeof(indices) + sizeof(vertices));
    _pTriangleStaticBuffer->SetName("Triangle Mesh");

    dx::StaticBufferUploadHeap uploadHeap(_pUploadHeap, _pTriangleStaticBuffer.get());
    _vertexBufferView = uploadHeap.AllocVertexBuffer(std::size(vertices), sizeof(glm::vec3), vertices).value();
    _indexBufferView = uploadHeap.AllocIndexBuffer(std::size(indices), sizeof(uint16_t), indices).value();
    uploadHeap.CommitUploadCommand();
}

void TriangleRenderer::CreateRootSignature() {
    CD3DX12_DESCRIPTOR_RANGE1 range(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
    _globalRootSignature.OnCreate(2, 0);
    _globalRootSignature.At(AccelerationStructureSlot).InitAsBufferSRV(0);         // s0
    _globalRootSignature.At(OutputRenderTarget).InitAsDescriptorTable({range});    // u0
    _globalRootSignature.Generate(_pDevice);

    _localRootSignature.OnCreate(1, 0);
    _localRootSignature.At(0).InitAsConstants(dx::SizeofInUint32<RayGenConstantBuffer>(), 0);    // b0
    _localRootSignature.Generate(_pDevice, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
}

void TriangleRenderer::CreateRayTracingPipelineStateObject() {
    ShaderManager *pShaderManager = ShaderManager::GetInstance();
    ShaderLoadInfo shaderLoadInfo;
    shaderLoadInfo.sourcePath = AssetProjectSetting::ToAssetPath("Shaders/HelloWorldRaytracing.hlsl");
    shaderLoadInfo.shaderType = dx::ShaderType::eLib;
    shaderLoadInfo.pDefineList = nullptr;
    D3D12_SHADER_BYTECODE byteCode = pShaderManager->LoadShaderByteCode(shaderLoadInfo);
    Assert(byteCode.pShaderBytecode != nullptr);

    CD3DX12_STATE_OBJECT_DESC rayTracingPipeline{D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE};
    CD3DX12_DXIL_LIBRARY_SUBOBJECT *pLib = rayTracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
    pLib->SetDXILLibrary(&byteCode);
    pLib->DefineExport(RayGenShaderName.data());
    pLib->DefineExport(ClosestHitShaderName.data());
    pLib->DefineExport(MissShaderName.data());

    CD3DX12_HIT_GROUP_SUBOBJECT *pHitGroup = rayTracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
    pHitGroup->SetClosestHitShaderImport(ClosestHitShaderName.data());
    pHitGroup->SetHitGroupExport(HitGroupName.data());
    pHitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

    auto *pShaderConfig = rayTracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
    size_t payloadSize = 4 * sizeof(float);    // float4
    size_t attributeSize = 2 * sizeof(float);
    pShaderConfig->Config(payloadSize, attributeSize);

    CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT
    *pLocalRootSignature = rayTracingPipeline.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
    pLocalRootSignature->SetRootSignature(_localRootSignature.GetRootSignature());
    CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT *rootSignatureAssociation = rayTracingPipeline.CreateSubobject<
        CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
    rootSignatureAssociation->SetSubobjectToAssociate(*pLocalRootSignature);
    rootSignatureAssociation->AddExport(RayGenShaderName.data());

    auto *pGlobalRootSignature = rayTracingPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
    pGlobalRootSignature->SetRootSignature(_globalRootSignature.GetRootSignature());

    auto *pPipelineConfig = rayTracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
    pPipelineConfig->Config(1);

    dx::NativeDevice *device = _pDevice->GetNativeDevice();
#if ENABLE_RAY_TRACING
    dx::ThrowIfFailed(device->CreateStateObject(rayTracingPipeline, IID_PPV_ARGS(&_pRayTracingPSO)));
#endif
}

void TriangleRenderer::CreateRayTracingOutputResource() {
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
    _rayTracingOutput.OnCreate(_pDevice, uavDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr);
    _rayTracingOutput.SetName("RayTracingOutput");

    if (_rayTracingOutputView.IsNull()) {
        _rayTracingOutputView = _pDevice->AllocDescriptor<dx::UAV>(1);
    }

    dx::NativeDevice *device = _pDevice->GetNativeDevice();
    D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc = {};
    viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    device->CreateUnorderedAccessView(_rayTracingOutput.GetResource(),
        nullptr,
        &viewDesc,
        _rayTracingOutputView.GetCpuHandle());
}

void TriangleRenderer::BuildAccelerationStructures() {
    _pASBuilder->OnCreate(_pDevice);

    dx::BottomLevelASGenerator bottomLevelAsGenerator;
    bottomLevelAsGenerator.AddGeometry(_vertexBufferView, DXGI_FORMAT_R32G32B32_FLOAT, _indexBufferView);
    _bottomLevelAs = bottomLevelAsGenerator.CommitBuildCommand(_pASBuilder);

    dx::TopLevelASGenerator topLevelAsGenerator;
    topLevelAsGenerator.AddInstance(_bottomLevelAs->GetResource(), glm::mat3x4(1.f), 0, 0);

    _topLevelAs = topLevelAsGenerator.CommitBuildCommand(_pASBuilder);
}
