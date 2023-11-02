#include "ASUploadHeap.h"
#include "Device.h"

namespace dx {

void ASUploadHeap::OnCreate(Device *pDevice) {
    _pDevice = pDevice;
    ID3D12Device *device = pDevice->GetNativeDevice();

    device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_pCommandAllocator));
    device->CreateCommandList(0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        _pCommandAllocator.Get(),
        nullptr,
        IID_PPV_ARGS(&_pCommandList));

    _pCommandAllocator->SetName(L"ASUploadHeap::CommandListAllocator");
    _pCommandList->SetName(L"ASUploadHeap::CommandList");
}

void ASUploadHeap::OnDestroy() {
    _pDevice = nullptr;
    _bufferSize = 0;
    _pScratchBuffer = nullptr;
    _pCommandList = nullptr;
    _pCommandAllocator = nullptr;
}

void ASUploadHeap::UpdateScratchBufferSize(size_t sizeInBytes) {
    _bufferSize = std::max(_bufferSize, sizeInBytes);
    _bufferSize = AlignUp(_bufferSize, 64);
}

auto ASUploadHeap::GetScratchBuffer() const -> ID3D12Resource * {
    if (_pScratchBuffer == nullptr || _pScratchBuffer->GetSize() < _bufferSize) {
        D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(_bufferSize,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        D3D12MA::Allocator *pAllocator = _pDevice->GetAllocator();
        D3D12MA::ALLOCATION_DESC allocationDesc = {};
        allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
        dx::ThrowIfFailed(pAllocator->CreateResource(&allocationDesc,
            &bufferDesc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            nullptr,
            &_pScratchBuffer,
            IID_NULL,
            nullptr));

        _pScratchBuffer->GetResource()->SetName(L"ASUploadHeap::ScratchBuffer");
    }
    return _pScratchBuffer->GetResource();
}

void ASUploadHeap::BuildRayTracingAccelerationStructure(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC &desc,
    ID3D12Resource *pResource) {

    _pCommandList->BuildRaytracingAccelerationStructure(&desc, 0, nullptr);
    _pCommandList->ResourceBarrier(1, RVPtr(CD3DX12_RESOURCE_BARRIER::UAV(pResource)));
}

void ASUploadHeap::Finish() {
    ThrowIfFailed(_pCommandList->Close());
    ID3D12CommandList *cmdList[] = {_pCommandList.Get()};
    _pDevice->GetCopyQueue()->ExecuteCommandLists(1, cmdList);
    ThrowIfFailed(_pCommandAllocator->Reset());
    ThrowIfFailed(_pCommandList->Reset(_pCommandAllocator.Get(), nullptr));
}

}    // namespace dx
