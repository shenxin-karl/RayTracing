#include "SimpleLighting.h"
#include "D3d12/ASBuilder.h"
#include "D3d12/BottomLevelASGenerator.h"
#include "D3d12/DescriptorManager.hpp"
#include "D3d12/Device.h"
#include "D3d12/StaticBuffer.h"
#include "D3d12/SwapChain.h"
#include "D3d12/TopLevelASGenerator.h"

void SimpleLighting::OnCreate(uint32_t numBackBuffer, HWND hwnd) {
    Renderer::OnCreate(numBackBuffer, hwnd);
    BuildGeometry();
    CreateRayTracingOutput();
    CreateRootSignature();
    CreateRayTracingPipeline();
    BuildAccelerationStructure();
}

void SimpleLighting::OnDestroy() {
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
}

void SimpleLighting::OnPreRender(GameTimer &timer) {
    Renderer::OnPreRender(timer);
}

void SimpleLighting::OnRender(GameTimer &timer) {
    Renderer::OnRender(timer);
}

void SimpleLighting::OnResize(uint32_t width, uint32_t height) {
    Renderer::OnResize(width, height);
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
        dx::NativeDevice *device = _pDevice->GetNativeDevice();
        D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc = {};
        viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        device->CreateUnorderedAccessView(_rayTracingOutput.GetResource(),
            nullptr,
            &viewDesc,
            _rayTracingOutputHandle.GetCpuHandle());
    }
}

void SimpleLighting::CreateRootSignature() {

}

void SimpleLighting::CreateRayTracingPipeline() {

}

void SimpleLighting::BuildAccelerationStructure() {
    _pASBuilder = std::make_unique<dx::ASBuilder>();
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
