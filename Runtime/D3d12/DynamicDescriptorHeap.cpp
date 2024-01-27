#include "DynamicDescriptorHeap.h"

#include "Context.h"
#include "DescriptorHandleArray.hpp"
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
    _pRootSignature = pRootSignature;
    _staleDescriptorTableBitMask = pRootSignature->GetDescriptorTableBitMask(_heapType);
    const auto &rootParamDescriptorTableInfo = pRootSignature->GetRootParamDescriptorTableInfo(_heapType);
    for (size_t rootIndex = 0; rootIndex < kMaxRootParameter; ++rootIndex) {
        bool enableBindless = rootParamDescriptorTableInfo[rootIndex].enableBindless;
        size_t capacity = rootParamDescriptorTableInfo[rootIndex].numDescriptor;
        _descriptorTableCache[rootIndex].Reset(enableBindless, capacity);
    }
}

void DynamicDescriptorHeap::Reset() {
    _numFreeHandles = 0;
    _currentCPUDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT);
    _currentGPUDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT);
    _pCurrentDescriptorHeap = nullptr;
    _pRootSignature = nullptr;
    _staleDescriptorTableBitMask.reset();
    _availableDescriptorHeaps = _descriptorHeapPool;
    for (std::size_t i = 0; i < kMaxRootParameter; ++i) {
        _descriptorTableCache[i].Reset(false, 0);
    }
}

void DynamicDescriptorHeap::StageDescriptors(size_t rootParameterIndex,
    size_t numDescriptors,
    const D3D12_CPU_DESCRIPTOR_HANDLE &baseDescriptor,
    size_t offset) {

    if (numDescriptors > 1) {
        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> handles;
        handles.reserve(numDescriptors);
        CD3DX12_CPU_DESCRIPTOR_HANDLE handle(baseDescriptor);
        for (size_t i = 0; i < numDescriptors; ++i) {
            handles.push_back(handle);
            handle.Offset(_descriptorHandleIncrementSize);
        }
        StageDescriptors(rootParameterIndex, handles, offset);
    } else {
        StageDescriptors(rootParameterIndex, baseDescriptor, offset);
    }
}

void DynamicDescriptorHeap::StageDescriptors(size_t rootParameterIndex,
    ReadonlyArraySpan<D3D12_CPU_DESCRIPTOR_HANDLE> handles,
    size_t offset) {

    if (rootParameterIndex >= kMaxRootParameter) {
        Exception::Throw("Invalid RootParameterIndex: {}", rootParameterIndex);
    }

    size_t capacity = _descriptorTableCache[rootParameterIndex].capacity;
    if (!_descriptorTableCache[rootParameterIndex].enableBindless && (offset + handles.Count()) > capacity) {
        Exception::Throw("OutOfRange! rootParameterIndex: {}", rootParameterIndex);
    }

    _descriptorTableCache[rootParameterIndex].Fill(offset, handles);
    _staleDescriptorTableBitMask.set(rootParameterIndex, true);
}

void DynamicDescriptorHeap::CommitStagedDescriptorForDraw(Context *pContext) {
    CommitDescriptorTables(pContext, &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable);
}

void DynamicDescriptorHeap::CommitStagedDescriptorForDispatch(Context *pContext) {
    CommitDescriptorTables(pContext, &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable);
}

void DynamicDescriptorHeap::EnsureCapacity(Context *pContext, size_t numDescriptorToCommit) {
    Assert(numDescriptorToCommit <= _numDescriptorsPreHeap);
    if (_pCurrentDescriptorHeap == nullptr || _numFreeHandles < numDescriptorToCommit) {
        _staleDescriptorTableBitMask = _pRootSignature->GetDescriptorTableBitMask(_heapType);
        _pCurrentDescriptorHeap = RequestDescriptorHeap();
        _currentCPUDescriptorHandle = _pCurrentDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        _currentGPUDescriptorHandle = _pCurrentDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
        _numFreeHandles = _numDescriptorsPreHeap;
        pContext->SetDescriptorHeap(_heapType, _pCurrentDescriptorHeap.Get());
        pContext->BindDescriptorHeaps();
    }
}

auto DynamicDescriptorHeap::CommitDescriptorHandleArray(const DescriptorHandleArray *pHandleArray)
    -> D3D12_GPU_DESCRIPTOR_HANDLE {

    NativeDevice *device = _pDevice->GetNativeDevice();
    UINT numDescriptors = static_cast<UINT>(pHandleArray->Count());
    const D3D12_CPU_DESCRIPTOR_HANDLE *pSrcHandle = pHandleArray->GetHandles().data();
    D3D12_CPU_DESCRIPTOR_HANDLE pDstDescriptorRangeStarts[] = {_currentCPUDescriptorHandle};
    UINT pDstDescriptorRangeSizes[] = {numDescriptors};

    device->CopyDescriptors(1,
        pDstDescriptorRangeStarts,
        pDstDescriptorRangeSizes,
        numDescriptors,
        pSrcHandle,
        nullptr,
        _heapType);

    D3D12_GPU_DESCRIPTOR_HANDLE handle = _currentGPUDescriptorHandle;
    _currentCPUDescriptorHandle.Offset(static_cast<INT>(numDescriptors),
        static_cast<UINT>(_descriptorHandleIncrementSize));
    _currentGPUDescriptorHandle.Offset(static_cast<INT>(numDescriptors),
        static_cast<UINT>(_descriptorHandleIncrementSize));
    return handle;
}

auto DynamicDescriptorHeap::ComputeStaleDescriptorCount() const -> size_t {
    if (_staleDescriptorTableBitMask.none()) {
        return 0;
    }

    size_t numStaleDescriptors = 0;
    for (std::size_t i = 0; i < kMaxRootParameter; ++i) {
        if (_staleDescriptorTableBitMask[i]) {
            numStaleDescriptors += _descriptorTableCache[i].count;
        }
    }
    return numStaleDescriptors;
}

void DynamicDescriptorHeap::BindDescriptorHeap(NativeCommandList *pCommandList) {
    if (_pCurrentDescriptorHeap != nullptr) {
        pCommandList->SetDescriptorHeaps(1, RVPtr(_pCurrentDescriptorHeap.Get()));
    }
}

void DynamicDescriptorHeap::DescriptorTableCache::Reset(bool enableBindless, size_t capacity) {
    this->enableBindless = enableBindless;
    this->capacity = capacity;
    this->count = 0;
    std::ranges::fill(cachedHandles, D3D12_CPU_DESCRIPTOR_HANDLE(0));
}

void DynamicDescriptorHeap::DescriptorTableCache::Fill(size_t offset,
    ReadonlyArraySpan<D3D12_CPU_DESCRIPTOR_HANDLE> handles) {
    size_t targetSize = offset + handles.Count();
    if (cachedHandles.size() < targetSize) {
        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> newCachedHandles;
        newCachedHandles.resize(targetSize);
        std::ranges::copy(cachedHandles, newCachedHandles.begin());
        std::swap(cachedHandles, newCachedHandles);
    }
    for (size_t i = 0; i < handles.Count(); ++i) {
        cachedHandles[i + offset] = handles[i];
    }
    count = std::max(count, offset + handles.Count());
}

void DynamicDescriptorHeap::CommitDescriptorTables(Context *pContext, CommitFunc commitFunc) {
    size_t numStaleDescriptors = ComputeStaleDescriptorCount();
    if (numStaleDescriptors == 0) {
        return;
    }

    NativeCommandList *pCmdList = pContext->GetCommandList();
    EnsureCapacity(pContext, numStaleDescriptors);

    ID3D12Device *device = _pDevice->GetNativeDevice();
    for (std::size_t rootIndex = 0; rootIndex < kMaxRootParameter; ++rootIndex) {
        if (!_staleDescriptorTableBitMask.test(rootIndex)) {
            continue;
        }

        UINT numDescriptors = static_cast<UINT>(_descriptorTableCache[rootIndex].count);
        D3D12_CPU_DESCRIPTOR_HANDLE *pSrcHandle = _descriptorTableCache[rootIndex].cachedHandles.data();
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
        (pCmdList->*commitFunc)(static_cast<UINT>(rootIndex), _currentGPUDescriptorHandle);
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