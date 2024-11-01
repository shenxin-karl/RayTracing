#include "DynamicBufferAllocator.h"
#include "Device.h"
#include "Foundation/StringUtil.h"

namespace dx {

DynamicBufferAllocator::DynamicBufferAllocator() : _pDevice(nullptr), _maxBufferSize(GetMByte(1)) {
}

DynamicBufferAllocator::~DynamicBufferAllocator() {
    OnDestroy();
}

void DynamicBufferAllocator::OnCreate(Device *pDevice) {
    _pDevice = pDevice;
}

void DynamicBufferAllocator::OnDestroy() {
    for (MemoryBlock &memoryBlock : _memoryBlockList) {
        ID3D12Resource *pResource = memoryBlock.pAllocation->GetResource();
        pResource->Unmap(0, nullptr);
    }
    _memoryBlockList.clear();
}

auto DynamicBufferAllocator::AllocVertexBuffer(size_t numOfVertices, size_t strideInBytes, const void *pInitData)
    -> D3D12_VERTEX_BUFFER_VIEW {
    size_t bufferSize = numOfVertices * strideInBytes;
    AllocInfo allocInfo = AllocBuffer(bufferSize);
    std::memcpy(allocInfo.pBuffer, pInitData, bufferSize);

    D3D12_VERTEX_BUFFER_VIEW view;
    view.BufferLocation = allocInfo.virtualAddress;
    view.SizeInBytes = bufferSize;
    view.StrideInBytes = strideInBytes;
    return view;
}

auto DynamicBufferAllocator::AllocIndexBuffer(size_t numOfIndex, size_t strideInBytes, const void *pInitData)
    -> D3D12_INDEX_BUFFER_VIEW {
    size_t bufferSize = numOfIndex * strideInBytes;
    AllocInfo allocInfo = AllocBuffer(bufferSize);
    std::memcpy(allocInfo.pBuffer, pInitData, bufferSize);

    D3D12_INDEX_BUFFER_VIEW view;
    view.BufferLocation = allocInfo.virtualAddress;
    view.SizeInBytes = bufferSize;
    if (strideInBytes == 2) {
        view.Format = DXGI_FORMAT_R16_UINT;
    } else if (strideInBytes == 4) {
        view.Format = DXGI_FORMAT_R32_UINT;
    } else {
        Assert("Invalid Index Format");
    }
    return view;
}

auto DynamicBufferAllocator::AllocConstantBuffer(size_t strideInBytes, const void *pInitData)
    -> D3D12_GPU_VIRTUAL_ADDRESS {

    size_t bufferSize = strideInBytes;
    AllocInfo allocInfo = AllocBuffer(AlignUp(bufferSize, 256), 256);
    std::memcpy(allocInfo.pBuffer, pInitData, strideInBytes);
    return allocInfo.virtualAddress;
}

auto DynamicBufferAllocator::AllocStructuredBuffer(size_t numOfVertices, size_t strideInBytes, const void *pInitData)
    -> D3D12_GPU_VIRTUAL_ADDRESS {

    size_t bufferSize = numOfVertices * strideInBytes;
    AllocInfo allocInfo = AllocBuffer(bufferSize);
    std::memcpy(allocInfo.pBuffer, pInitData, bufferSize);
    return allocInfo.virtualAddress;
}

auto DynamicBufferAllocator::AllocBuffer(size_t bufferSize, size_t addressAlignment) -> AllocInfo {
    AllocInfo allocInfo = {};
    for (MemoryBlock &memoryBlock : _memoryBlockList) {
        uint8_t *pCurrent = reinterpret_cast<uint8_t *>(
            AlignUp<std::ptrdiff_t>(reinterpret_cast<std::ptrdiff_t>(memoryBlock.pCurrent), addressAlignment));
        if ((memoryBlock.pEnd - pCurrent) > bufferSize) {
            size_t offset = pCurrent - memoryBlock.pBegin;
            allocInfo.pBuffer = pCurrent;
            allocInfo.virtualAddress = memoryBlock.virtualAddress + offset;
            memoryBlock.pCurrent = pCurrent + bufferSize;
            return allocInfo;
        }
    }

    while (_maxBufferSize < bufferSize) {
        _maxBufferSize *= 2;
    }

    MemoryBlock &block = _memoryBlockList.emplace_back();
    D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(_maxBufferSize);

    D3D12MA::Allocator *pAllocator = _pDevice->GetAllocator();
    D3D12MA::ALLOCATION_DESC allocationDesc = {};
    allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
    ThrowIfFailed(pAllocator->CreateResource(&allocationDesc,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        &block.pAllocation,
        IID_NULL,
        nullptr));

    ID3D12Resource *pResource = block.pAllocation->GetResource();
    std::wstring name = nstd::to_wstring(fmt::format("DynamicBufferAllocator::Resource_{}", _memoryBlockList.size()));
    pResource->SetName(name.data());

    pResource->Map(0, nullptr, (void **)&block.pBegin);
    block.pEnd = block.pBegin + bufferDesc.Width;
    block.pCurrent = block.pBegin;
    block.virtualAddress = pResource->GetGPUVirtualAddress();

    allocInfo.pBuffer = block.pCurrent;
    allocInfo.virtualAddress = block.virtualAddress;
    block.pCurrent = block.pBegin + bufferSize;
    return allocInfo;
}

void DynamicBufferAllocator::Reset() {
    for (MemoryBlock &memoryBlock : _memoryBlockList) {
        memoryBlock.pCurrent = memoryBlock.pBegin;
    }
}

}    // namespace dx