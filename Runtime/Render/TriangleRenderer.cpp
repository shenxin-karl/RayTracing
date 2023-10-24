#include "TriangleRenderer.h"
#include "D3d12/Context.h"
#include "D3d12/DescriptorManager.hpp"
#include "D3d12/Device.h"
#include "D3d12/FrameResource.h"
#include "D3d12/FrameResourceRing.h"
#include "D3d12/ShaderCompiler.h"
#include "D3d12/StaticBuffer.h"
#include "D3d12/SwapChain.h"
#include "D3d12/UploadHeap.h"
#include "Foundation/GameTimer.h"
#include "Foundation/Logger.h"
#include "ShaderLoader/ShaderManager.h"
#include "Utils/AssetProjectSetting.h"

enum RootIndex {
    AccelerationStructureSlot = 0,
    OutputRenderTarget = 1,
};

static std::wstring_view RayGenShaderName = L"MyRaygenShader";
static std::wstring_view ClosestHitShaderName = L"MyClosestHitShader";
static std::wstring_view MissShaderName = L"MyMissShader";
static std::wstring_view HitGroupName = L"MyHitGroup";

void TriangleRenderer::OnCreate(uint32_t numBackBuffer, HWND hwnd) {
    Renderer::OnCreate(numBackBuffer, hwnd);
    CreateGeometry();
    CreateRootSignature();
    CreateRayTracingPipelineStateObject();
    CreateRayTracingOutputResource();
    BuildAccelerationStructures();
}

void TriangleRenderer::OnDestroy() {
    _pTriangleStaticBuffer->OnDestroy();
    Renderer::OnDestroy();
}

void TriangleRenderer::OnRender(GameTimer &timer) {
    Renderer::OnRender(timer);

    static uint64_t frameCount = 0;
    if (static_cast<uint64_t>(timer.GetTotalTime()) > frameCount) {
        Logger::Info("fps {}", timer.GetFPS());
        frameCount = static_cast<uint64_t>(timer.GetTotalTime());
    }

    _pFrameResourceRing->OnBeginFrame();
    dx::FrameResource &frameResource = _pFrameResourceRing->GetCurrentFrameResource();
    std::shared_ptr<dx::GraphicsContext> pGraphicsCtx = frameResource.AllocGraphicsContext();

    CD3DX12_VIEWPORT viewport = CD3DX12_VIEWPORT(0.f, 0.f, static_cast<float>(_width), static_cast<float>(_height));
    D3D12_RECT scissor = {0, 0, static_cast<LONG>(_width), static_cast<LONG>(_height)};
    pGraphicsCtx->SetViewport(viewport);
    pGraphicsCtx->SetScissor(scissor);
    pGraphicsCtx->SetRenderTargets(_pSwapChain->GetCurrentBackBufferRTV());
    pGraphicsCtx->Transition(_pSwapChain->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET);
    pGraphicsCtx->FlushResourceBarriers();

    glm::vec4 color = {
        std::sin(timer.GetTotalTime()) * 0.5f + 0.5f,
        std::cos(timer.GetTotalTime()) * 0.5f + 0.5f,
        0.f,
        1.f,
    };

    pGraphicsCtx->ClearRenderTargetView(_pSwapChain->GetCurrentBackBufferRTV(), color);
    pGraphicsCtx->Transition(_pSwapChain->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT);
    frameResource.ExecuteContexts(pGraphicsCtx.get());

    _pFrameResourceRing->OnEndFrame();
    _pSwapChain->Present();
}

void TriangleRenderer::CreateGeometry() {
    _pTriangleStaticBuffer = std::make_shared<dx::StaticBuffer>();
    uint16_t indices[] = {0, 1, 2};

    float depthValue = 1.0;
    float offset = 0.7f;
    glm::vec3 vertices[] = {{0, -offset, depthValue}, {-offset, offset, depthValue}, {offset, offset, depthValue}};

    _pTriangleStaticBuffer->OnCreate(_pDevice.get(), sizeof(indices) + sizeof(vertices));
    dx::StaticBufferUploadHeap uploadHeap(*_pTriangleStaticBuffer, *_pUploadHeap);
    _vertexBufferView = uploadHeap.AllocVertexBuffer(std::size(vertices), sizeof(glm::vec3), vertices).value();
    _indexBufferView = uploadHeap.AllocIndexBuffer(std::size(indices), sizeof(uint16_t), indices).value();
    uploadHeap.DoUpload();
}

void TriangleRenderer::CreateRootSignature() {
    CD3DX12_DESCRIPTOR_RANGE range(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
    _globalRootSignature.Reset(2, 0);
    _globalRootSignature.At(AccelerationStructureSlot).InitAsBufferSRV(0);         // s0
    _globalRootSignature.At(OutputRenderTarget).InitAsDescriptorTable({range});    // u0
    _globalRootSignature.Finalize(_pDevice.get());

    _localRootSignature.Reset(1, 0);
    _localRootSignature.At(0).InitAsBufferCBV(0);    // b0
    _localRootSignature.Finalize(_pDevice.get(), D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
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

    auto *pLocalRootSignature = rayTracingPipeline.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
    pLocalRootSignature->SetRootSignature(_localRootSignature.GetRootSignature());

    auto *pGlobalRootSignature = rayTracingPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
    pGlobalRootSignature->SetRootSignature(_globalRootSignature.GetRootSignature());

    auto *pPipelineConfig = rayTracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
    pPipelineConfig->Config(1);

    dx::NativeDevice *device = _pDevice->GetNativeDevice();
    dx::ThrowIfFailed(device->CreateStateObject(rayTracingPipeline, IID_PPV_ARGS(&_pRayTracingPSO)));
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
    _rayTracingOutput.OnCreate(_pDevice.get(), uavDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr);
    _rayTracingOutput.SetName("RayTracingOutput");

    if (_rayTracingOutputView.IsNull()) {
        _rayTracingOutputView = dx::DescriptorManager::Alloc<dx::UAV>();
    }

    dx::NativeDevice *device = _pDevice->GetNativeDevice();
    D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc = {};
    viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    device->CreateUnorderedAccessView(_rayTracingOutput.GetResource(),
        nullptr,
        &viewDesc,
        _rayTracingOutputView.GetCpuHandle());
}

static void AllocateUploadBuffer(ID3D12Device *pDevice,
    void *pData,
    UINT64 datasize,
    ID3D12Resource **ppResource,
    const wchar_t *resourceName = nullptr) {
    auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(datasize);
    dx::ThrowIfFailed(pDevice->CreateCommittedResource(&uploadHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(ppResource)));

    if (resourceName) {
        (*ppResource)->SetName(resourceName);
    }
    void *pMappedData;
    (*ppResource)->Map(0, nullptr, &pMappedData);
    memcpy(pMappedData, pData, datasize);
    (*ppResource)->Unmap(0, nullptr);
}

void TriangleRenderer::BuildAccelerationStructures() {
    _pUploadHeap->DoUpload();
    dx::NativeDevice *device = _pDevice->GetNativeDevice();
    dx::NativeCommandList *pCmdList = _pUploadHeap->GetCopyCommandList();
    ID3D12CommandAllocator *pCmdAllocator = _pUploadHeap->GetCommandAllocator();

    auto CreateUAVBuffer = [=](size_t bufferSize, D3D12_RESOURCE_STATES initResourceStates, std::wstring_view name) {
        dx::WRL::ComPtr<D3D12MA::Allocation> pAllocation;
        D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
        D3D12MA::Allocator *pAllocator = _pDevice->GetAllocator();
        D3D12MA::ALLOCATION_DESC allocationDesc = {};
        allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
        dx::ThrowIfFailed(pAllocator->CreateResource(&allocationDesc,
            &bufferDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            &pAllocation,
            IID_NULL,
            nullptr));
        if (pAllocation != nullptr) {
            pAllocation->GetResource()->SetName(name.data());
        }
        return pAllocation;
    };

    D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
    geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geometryDesc.Triangles.IndexBuffer = _indexBufferView.BufferLocation;
    geometryDesc.Triangles.IndexCount = _indexBufferView.SizeInBytes / sizeof(uint16_t);
    geometryDesc.Triangles.IndexFormat = _indexBufferView.Format;
    geometryDesc.Triangles.Transform3x4 = 0;
    geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
    geometryDesc.Triangles.VertexCount = _vertexBufferView.SizeInBytes / _vertexBufferView.StrideInBytes;
    geometryDesc.Triangles.VertexBuffer.StartAddress = _vertexBufferView.BufferLocation;
    geometryDesc.Triangles.VertexBuffer.StrideInBytes = _vertexBufferView.StrideInBytes;
    // Mark the geometry as opaque.
    // PERFORMANCE TIP: mark geometry as opaque whenever applicable as it can enable important ray processing optimizations.
    // Note: When rays encounter opaque geometry an any hit shader will not be executed whether it is present or not.
    geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

    // Get required sizes for an acceleration structure.
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS
    buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = {};
    topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    topLevelInputs.Flags = buildFlags;
    topLevelInputs.NumDescs = 1;
    topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPreBuildInfo = {};
    device->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPreBuildInfo);
    Assert(topLevelPreBuildInfo.ResultDataMaxSizeInBytes > 0);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs = topLevelInputs;
    bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    bottomLevelInputs.pGeometryDescs = &geometryDesc;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPreBuildInfo = {};
    device->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPreBuildInfo);
    Assert(bottomLevelPreBuildInfo.ResultDataMaxSizeInBytes > 0);

    dx::WRL::ComPtr<D3D12MA::Allocation> pScratchResource = CreateUAVBuffer(
        std::max<size_t>(topLevelPreBuildInfo.ScratchDataSizeInBytes, bottomLevelPreBuildInfo.ScratchDataSizeInBytes),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        L"ScratchResource");

    _pBottomLevelAccelerationStructure = CreateUAVBuffer(bottomLevelPreBuildInfo.ResultDataMaxSizeInBytes,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
        L"BottomLevelAccelerationStructure");
    _pTopLevelAccelerationStructure = CreateUAVBuffer(topLevelPreBuildInfo.ResultDataMaxSizeInBytes,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
        L"TopLevelAccelerationStructure");

    // Create an instance desc for the bottom-level acceleration structure.
    Microsoft::WRL::ComPtr<ID3D12Resource> instanceDescs;
    D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
    instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1;
    instanceDesc.InstanceMask = 1;
    instanceDesc.AccelerationStructure = _pBottomLevelAccelerationStructure->GetResource()->GetGPUVirtualAddress();
    AllocateUploadBuffer(device, &instanceDesc, sizeof(instanceDesc), &instanceDescs, L"InstanceDescs");

    // ���ײ���ٽṹ�� desc �ṹ��
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
    {
        bottomLevelBuildDesc.Inputs = bottomLevelInputs;
        bottomLevelBuildDesc.DestAccelerationStructureData = _pBottomLevelAccelerationStructure->GetResource()->GetGPUVirtualAddress();
        bottomLevelBuildDesc.ScratchAccelerationStructureData = pScratchResource->GetResource()->GetGPUVirtualAddress();
    }

    // ��䶥����ٽṹ�� desc �ṹ��
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
    {
        topLevelInputs.InstanceDescs = instanceDescs->GetGPUVirtualAddress();
        topLevelBuildDesc.Inputs = topLevelInputs;
        topLevelBuildDesc.DestAccelerationStructureData = _pTopLevelAccelerationStructure->GetResource()->GetGPUVirtualAddress();
        topLevelBuildDesc.ScratchAccelerationStructureData = pScratchResource->GetResource()->GetGPUVirtualAddress();
    }

	// �������ٽṹ
    pCmdList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);
    pCmdList->ResourceBarrier(1, dx::RVPtr(CD3DX12_RESOURCE_BARRIER::UAV(_pBottomLevelAccelerationStructure->GetResource())));
    pCmdList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);
    dx::ThrowIfFailed(pCmdList->Close());

    ID3D12CommandList *commandLists[] = { pCmdList }; 
    _pDevice->GetCopyQueue()->ExecuteCommandLists(1, commandLists);
    _pDevice->WaitForGPUFlush(D3D12_COMMAND_LIST_TYPE_COPY);

    dx::ThrowIfFailed(pCmdList->Reset(pCmdAllocator, nullptr));
}