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

void DynamicDescriptorHeap::ParseRootSignature(const RootSignature *pRootSignature) {
    _descriptorTableBitMask = pRootSignature->GetDescriptorTableBitMask(_heapType);
    _staleDescriptorTableBitMask = _descriptorTableBitMask;

    size_t offset = 0;
    const auto &numDescriptorPreTable = pRootSignature->GetNumDescriptorPreTable(_heapType);
    for (size_t rootIndex = 0; rootIndex < kMaxRootParameter; ++rootIndex) {
        if (!_descriptorTableBitMask.test(rootIndex)) {
            continue;
        }

        size_t count = numDescriptorPreTable[rootIndex];
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

    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> handles;
    handles.reserve(numDescriptors);

    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(baseDescriptor);
	for (size_t i = 0; i < numDescriptors; ++i) {
		handles.push_back(handle);
        handle.Offset(_descriptorHandleIncrementSize);
	}
    StageDescriptors(rootParameterIndex, handles, offset);
}

void DynamicDescriptorHeap::StageDescriptors(size_t rootParameterIndex,
	ReadonlyArraySpan<D3D12_CPU_DESCRIPTOR_HANDLE> handles, size_t offset) {

    if (!_descriptorTableBitMask.test(rootParameterIndex)) {
	    Exception::Throw("Invalid RootParameterIndex: {}", rootParameterIndex);
    }
    if (handles.Count() >= _numDescriptorsPreHeap || rootParameterIndex >= kMaxRootParameter ||
        (offset + handles.Count()) > _descriptorTableCache[rootParameterIndex].numDescriptors) {
        Exception::Throw("Out of range!");
    }

    _staleDescriptorTableBitMask.set(rootParameterIndex, true);
    D3D12_CPU_DESCRIPTOR_HANDLE *pBaseHandle = _descriptorTableCache[rootParameterIndex].pBaseHandle;
    for (size_t i = 0; i < handles.Count(); ++i) {
        pBaseHandle[offset + i] = handles[i];
    }
}

auto DynamicDescriptorHeap::ComputeStaleDescriptorCount() const -> size_t {
    if (_staleDescriptorTableBitMask.none()) {
        return 0;
    }

    size_t numStaleDescriptors = 0;
    for (std::size_t i = 0; i < kMaxRootParameter; ++i) {
        int flag = _staleDescriptorTableBitMask.test(i);
        numStaleDescriptors += flag * _descriptorTableCache[i].numDescriptors;
    }
    return numStaleDescriptors;
}

void DynamicDescriptorHeap::CommitDescriptorTables(NativeCommandList *pCommandList, CommitFunc commitFunc) {
    size_t numStaleDescriptors = ComputeStaleDescriptorCount();
    if (numStaleDescriptors == 0) {
        return;
    }

    if (_pCurrentDescriptorHeap == nullptr || _numFreeHandles < numStaleDescriptors) {
        _staleDescriptorTableBitMask = _descriptorTableBitMask;
        _pCurrentDescriptorHeap = RequestDescriptorHeap();
        _currentCPUDescriptorHandle = _pCurrentDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        _currentGPUDescriptorHandle = _pCurrentDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
        _numFreeHandles = _numDescriptorsPreHeap;
        pCommandList->SetDescriptorHeaps(1, RVPtr(_pCurrentDescriptorHeap.Get()));
    }

    ID3D12Device *device = _pDevice->GetNativeDevice();
    for (std::size_t rootIndex = 0; rootIndex < kMaxRootParameter; ++rootIndex) {
        if (!_staleDescriptorTableBitMask.test(rootIndex))
            continue;

        UINT numDescriptors = static_cast<UINT>(_descriptorTableCache[rootIndex].numDescriptors);
        auto *pSrcHandle = _descriptorTableCache[rootIndex].pBaseHandle;
        D3D12_CPU_DESCRIPTOR_HANDLE pDstDescriptorRangeStarts[] = {_currentCPUDescriptorHandle};
        UINT pDstDescriptorRangeSizes[] = {numDescriptors};

        device->CopyDescriptors(1,
            pDstDescriptorRangeStarts,
            pDstDescriptorRangeSizes,
            numDescriptors,
            pSrcHandle,
            nullptr,
            _heapType);

        // Bind to the Command list
        _numFreeHandles -= numDescriptors;
        (pCommandList->*commitFunc)(static_cast<UINT>(rootIndex), _currentGPUDescriptorHandle);
        _currentCPUDescriptorHandle.Offset(static_cast<INT>(numDescriptors),
            static_cast<UINT>(_descriptorHandleIncrementSize));
        _currentGPUDescriptorHandle.Offset(static_cast<INT>(numDescriptors),
            static_cast<UINT>(_descriptorHandleIncrementSize));
    }
    _staleDescriptorTableBitMask.reset();
}

auto DynamicDescriptorHeap::RequestDescriptorHeap() -> WRL::ComPtr<ID3D12DescriptorHeap> {
    WRL::ComPtr<ID3D12DescriptorHeap> pDescriptorHeap;
    if (!_availableDescriptorHeaps.empty()) {
        pDescriptorHeap = _availableDescriptorHeaps.front();
        _availableDescriptorHeaps.pop();
    } else {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        heapDesc.NodeMask = 0;
        heapDesc.NumDescriptors = static_cast<UINT>(_numDescriptorsPreHeap);
        heapDesc.Type = _heapType;
        ThrowIfFailed(_pDevice->GetNativeDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&pDescriptorHeap)));
        _descriptorHeapPool.push(pDescriptorHeap);
    }
    return pDescriptorHeap;
}

}    // namespace dx