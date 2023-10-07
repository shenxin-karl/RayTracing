#include "DynamicDescriptorHeap.h"
#include "Device.h"
#include "RootSignature.h"

namespace dx {

DynamicDescriptorHeap::DynamicDescriptorHeap(Device *pDevice,
    D3D12_DESCRIPTOR_HEAP_TYPE heapType,
    size_t numDescriptorsPerHeap) {

    auto device = pDevice->GetNativeDevice();
    _pDevice = pDevice;
    _numDescriptorsPreHeap = numDescriptorsPerHeap;
    _descriptorHandleIncrementSize = device->GetDescriptorHandleIncrementSize(heapType);
    _heapType = heapType;
    Reset();
}

void DynamicDescriptorHeap::ParseRootSignature(RootSignature *pRootSignature) {
    _descriptorTableBitMask = pRootSignature->GetDescriptorTableBitMask(_heapType);
    _staleDescriptorTableBitMask = _descriptorTableBitMask;

    size_t offset = 0;
    const auto &numDescriptorPreTable = pRootSignature->GetNumDescriptorPreTable(_heapType);
    for (size_t rootIndex = 0; rootIndex < kMaxRootParameter; ++rootIndex) {
        if (!_descriptorTableBitMask.test(rootIndex)) {
            continue;
        }

        uint8_t count = numDescriptorPreTable[rootIndex];
        _descriptorTableCache[rootIndex].pBaseHandle = _descriptorHandleCache.data() + offset;
        _descriptorTableCache[rootIndex].numDescriptors = count;
        offset += count;
    }

    // clear cache descriptor handle
    for (size_t i = 0; i < offset; ++i) {
        _descriptorHandleCache[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT);
    }
}

void DynamicDescriptorHeap::Reset() {
    _numFreeHandles = 0;
    _currentCPUDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT);
    _currentGPUDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT);
    _pCurrentDescriptorHeap = nullptr;
    _descriptorTableBitMask.reset();
    _staleDescriptorTableBitMask.reset();
    _availableDescriptorHeaps = _descriptorHeapPool;
    for (std::size_t i = 0; i < kMaxRootParameter; ++i) {
        _descriptorTableCache[i] = {0, nullptr};
        _descriptorHandleCache[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT);
    }
}

void DynamicDescriptorHeap::StageDescriptors(size_t rootParameterIndex,
    size_t numDescriptors,
    const D3D12_CPU_DESCRIPTOR_HANDLE &baseDescriptor,
    size_t offset) {

    if (numDescriptors >= _numDescriptorsPreHeap || rootParameterIndex >= kMaxRootParameter ||
        (offset + numDescriptors) >= _descriptorTableCache[rootParameterIndex].numDescriptors) {
        Assert("Out of range!");
    }

    _staleDescriptorTableBitMask.set(rootParameterIndex, true);
    D3D12_CPU_DESCRIPTOR_HANDLE *pBaseHandle = _descriptorTableCache[rootParameterIndex].pBaseHandle;
    CD3DX12_CPU_DESCRIPTOR_HANDLE descriptor(baseDescriptor);
    for (size_t i = 0; i < numDescriptors; ++i) {
        pBaseHandle[offset + i] = descriptor;
        descriptor.Offset(_descriptorHandleIncrementSize);
    }
}

}    // namespace dx