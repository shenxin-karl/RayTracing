#include "ASBuilder.h"
#include "Device.h"

namespace dx {

ASBuilder::~ASBuilder() {
    OnDestroy();
}

void ASBuilder::OnCreate(Device *pDevice) {
    _pDevice = pDevice;
    ID3D12Device *device = pDevice->GetNativeDevice();

    device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_pCommandAllocator));
    device->CreateCommandList(0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        _pCommandAllocator.Get(),
        nullptr,
        IID_PPV_ARGS(&_pCommandList));

    _pCommandAllocator->SetName(L"ASBuilder::CommandListAllocator");
    _pCommandList->SetName(L"ASBuilder::CommandList");
}

void ASBuilder::OnDestroy() {
    _pDevice = nullptr;
    _scratchBufferSize = 0;
    _pScratchBuffer = nullptr;
    _pCommandList = nullptr;
    _pCommandAllocator = nullptr;

    if (_pInstanceBufferAddress != nullptr) {
        _pInstanceBuffer->GetResource()->Unmap(0, nullptr);
    }
    _instanceCount = 0;
    _pInstanceBuffer = nullptr;
    _pInstanceBufferAddress = nullptr;
}

auto ASBuilder::GetScratchBuffer() const -> ID3D12Resource * {
    if (_pScratchBuffer == nullptr || _pScratchBuffer->GetSize() < _scratchBufferSize) {
        D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(_scratchBufferSize,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        D3D12MA::Allocator *pAllocator = _pDevice->GetAllocator();
        D3D12MA::ALLOCATION_DESC allocationDesc = {};
        allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
        ThrowIfFailed(pAllocator->CreateResource(&allocationDesc,
            &bufferDesc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            nullptr,
            &_pScratchBuffer,
            IID_NULL,
            nullptr));

        _pScratchBuffer->GetResource()->SetName(L"ASBuilder::ScratchBuffer");
    }
    return _pScratchBuffer->GetResource();
}

auto ASBuilder::GetInstanceBuffer() const -> std::span<D3D12_RAYTRACING_INSTANCE_DESC> {
    size_t bufferSize = AlignUp(_instanceCount * sizeof(D3D12_RAYTRACING_INSTANCE_DESC),
        D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    if (_pInstanceBuffer == nullptr || _pInstanceBuffer->GetSize() < bufferSize) {
        if (_pInstanceBufferAddress != nullptr) {
            _pInstanceBuffer->GetResource()->Unmap(0, nullptr);
            _pInstanceBufferAddress = nullptr;
        }
        D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
        D3D12MA::Allocator *pAllocator = _pDevice->GetAllocator();
        D3D12MA::ALLOCATION_DESC allocationDesc = {};
        allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
        ThrowIfFailed(pAllocator->CreateResource(&allocationDesc,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            &_pInstanceBuffer,
            IID_NULL,
            nullptr));
        _pInstanceBuffer->GetResource()->SetName(L"ASBuilder::InstanceBuffer");
    }

    if (_pInstanceBufferAddress == nullptr) {
        _pInstanceBuffer->GetResource()->Map(0, nullptr, reinterpret_cast<void **>(&_pInstanceBufferAddress));
    }
    return {_pInstanceBufferAddress, _instanceCount};
}

auto ASBuilder::GetInstanceBufferGPUAddress() const -> D3D12_GPU_VIRTUAL_ADDRESS {
    return _pInstanceBuffer->GetResource()->GetGPUVirtualAddress();
}

auto ASBuilder::GetScratchBufferGPUAddress() const -> D3D12_GPU_VIRTUAL_ADDRESS {
    return GetScratchBuffer()->GetGPUVirtualAddress();
}

void ASBuilder::BuildRayTracingAccelerationStructure(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC &desc,
    ID3D12Resource *pResource) {

    _pCommandList->BuildRaytracingAccelerationStructure(&desc, 0, nullptr);
    _pCommandList->ResourceBarrier(1, RVPtr(CD3DX12_RESOURCE_BARRIER::UAV(pResource)));
}

void ASBuilder::BuildFinish() {
    ThrowIfFailed(_pCommandList->Close());
    ID3D12CommandList *cmdList[] = {_pCommandList.Get()};
    _pDevice->GetCopyQueue()->ExecuteCommandLists(1, cmdList);
    _pDevice->WaitForGPUFlush(D3D12_COMMAND_LIST_TYPE_COPY);
    ThrowIfFailed(_pCommandAllocator->Reset());
    ThrowIfFailed(_pCommandList->Reset(_pCommandAllocator.Get(), nullptr));
}

}    // namespace dx
