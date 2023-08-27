#include "DescriptorPage.h"
#include "Device.h"

namespace dx {

DescriptorPage::DescriptorPage(Device *pDevice, D3D12_DESCRIPTOR_HEAP_TYPE heapType, size_t numDescriptorPrePage) {
    auto device = pDevice->GetDevice();
    _numFreeHandle = numDescriptorPrePage;
    _numDescriptorPrePage = numDescriptorPrePage;
    _descriptorIncrementSize = device->GetDescriptorHandleIncrementSize(heapType);

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heapDesc.NodeMask = 0;
    heapDesc.NumDescriptors = static_cast<UINT>(numDescriptorPrePage);
    heapDesc.Type = heapType;
    ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_pDescriptorHeap)));
    _baseHandle = _pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    AddFreeBlock(0, numDescriptorPrePage);
}

DescriptorPage::~DescriptorPage() {
    for (std::atomic_size_t *pRefCount : _allRefCountList) {
	    delete pRefCount;
    }
    _allRefCountList.clear();
    _availableRefCountList.clear();
}

auto DescriptorPage::Alloc(size_t numDescriptor) -> std::optional<DescriptorHandle> {
    std::lock_guard lock(_allocMutex);
    if (numDescriptor > _numFreeHandle) {
        return std::nullopt;
    }

    auto sizeIter = _freeBlockSizeMap.lower_bound(numDescriptor);
    if (sizeIter == _freeBlockSizeMap.end()) {
        return std::nullopt;
    }

    size_t size = sizeIter->first;
    size_t offset = sizeIter->second->first;
    CD3DX12_CPU_DESCRIPTOR_HANDLE handle = _baseHandle;
    handle.Offset(offset, _descriptorIncrementSize);

    std::atomic_size_t *pRefCount = nullptr;
    if (!_availableRefCountList.empty()) {
        pRefCount = _availableRefCountList.back();
        _availableRefCountList.pop_back();
    } else {
        pRefCount = new std::atomic_size_t;
        _allRefCountList.push_back(pRefCount);
    }
    pRefCount->store(1);

    DescriptorHandle descriptorHandle = {handle, numDescriptor, _descriptorIncrementSize, pRefCount, this};

    _numFreeHandle -= numDescriptor;
    _freeBlockSizeMap.erase(sizeIter);
    _freeBlockOffsetMap.erase(offset);

    size_t blockRemainingSize = size - numDescriptor;
    if (blockRemainingSize > 0) {
        size_t newOffset = offset + numDescriptor;
        AddFreeBlock(newOffset, blockRemainingSize);
    }

    return descriptorHandle;
}

void DescriptorPage::Free(D3D12_CPU_DESCRIPTOR_HANDLE handle, size_t size, std::atomic_size_t *pRefCount) {
    Assert(pRefCount->load() == 0);
    StaleDescriptorBlock staleInfo = {ComputeOffset(handle), size};
    std::lock_guard lock(_allocMutex);
    _staleDescriptorBlockList.push_back(staleInfo);
    _availableRefCountList.push_back(pRefCount);
}

void DescriptorPage::ReleaseStaleDescriptors() {
    std::lock_guard lock(_allocMutex);
    for (StaleDescriptorBlock &staleFreeBlock : _staleDescriptorBlockList) {
	    FreeBlock(staleFreeBlock.offset, staleFreeBlock.size);
    }
    _staleDescriptorBlockList.clear();
}

void DescriptorPage::AddFreeBlock(size_t offset, size_t size) {
    FreeBlockOffsetInfo block;
    block.offset = offset;
    block.size = size;
    block.sizeIter = {};
    auto &&[freeBlockOffsetIter, _] = _freeBlockOffsetMap.emplace(offset, block);
    auto sizeIter = _freeBlockSizeMap.emplace(size, freeBlockOffsetIter);
    freeBlockOffsetIter->second.sizeIter = sizeIter;
}

void DescriptorPage::FreeBlock(size_t offset, size_t size) {
    auto nextOffsetIter = _freeBlockOffsetMap.lower_bound(offset);
    size_t newOffset = offset;
    size_t newSize = size;

    //        prev             current               next
	// |----------------|------------------|--------------------|
    if (nextOffsetIter != _freeBlockOffsetMap.begin()) {
	    auto prevOffsetIter = nextOffsetIter;
        --prevOffsetIter;
        if (prevOffsetIter->first + prevOffsetIter->second.size == offset) {
	        newOffset = prevOffsetIter->first;
            newSize += prevOffsetIter->second.size;
            _freeBlockSizeMap.erase(prevOffsetIter->second.sizeIter);
            _freeBlockOffsetMap.erase(prevOffsetIter);
        }
    }

    if (nextOffsetIter != _freeBlockOffsetMap.end()) {
	    if (offset + size == nextOffsetIter->first) {
		    newSize += nextOffsetIter->second.size;
            _freeBlockSizeMap.erase(nextOffsetIter->second.sizeIter);
            _freeBlockOffsetMap.erase(nextOffsetIter);
	    }
    }
	AddFreeBlock(newOffset, newSize);
    _numFreeHandle += size;
}

auto DescriptorPage::ComputeOffset(D3D12_CPU_DESCRIPTOR_HANDLE handle) const -> size_t {
    return (handle.ptr - _baseHandle.ptr) / _descriptorIncrementSize;
}

}    // namespace dx
