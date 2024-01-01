#pragma once
#include <mutex>
#include <optional>
#include <map>
#include <unordered_map>
#include "D3dStd.h"
#include "DescriptorHandle.h"

namespace dx {

class DescriptorPage : NonCopyable {
public:
	DescriptorPage(Device *pDevice, D3D12_DESCRIPTOR_HEAP_TYPE heapType, size_t numDescriptorPrePage);
	~DescriptorPage();
public:
	auto Alloc(size_t numDescriptor) -> std::optional<DescriptorHandle>;
	void Free(D3D12_CPU_DESCRIPTOR_HANDLE handle, size_t size, std::atomic_size_t *pRefCount);
	void ReleaseStaleDescriptors();
public:
	void AddFreeBlock(size_t offset, size_t size);
	void FreeBlock(size_t offset, size_t size);
	auto ComputeOffset(D3D12_CPU_DESCRIPTOR_HANDLE handle) const -> size_t;
private:
	struct StaleDescriptorBlock {
		size_t offset;
		size_t size;
	};

	struct FreeBlockOffsetInfo;
	using FreeBlockOffsetMap = std::map<size_t, FreeBlockOffsetInfo>;
	using FreeBlockSizeMap = std::multimap<size_t, FreeBlockOffsetMap::iterator>;
	struct FreeBlockOffsetInfo {
		size_t offset;
		size_t size;
		FreeBlockSizeMap::iterator sizeIter;
	};

	using AllocationRefCountList = std::vector<std::atomic_size_t *>;
	using StaleDescriptorBlockList = std::vector<StaleDescriptorBlock>;
private:
	// clang-format off
	size_t								_numFreeHandle;
	size_t								_numDescriptorPrePage;
	size_t								_descriptorIncrementSize;
	CD3DX12_CPU_DESCRIPTOR_HANDLE		_baseHandle;
	mutable std::mutex					_allocMutex;
	WRL::ComPtr<ID3D12DescriptorHeap>	_pDescriptorHeap;
	StaleDescriptorBlockList			_staleDescriptorBlockList;
	FreeBlockOffsetMap					_freeBlockOffsetMap;
	FreeBlockSizeMap					_freeBlockSizeMap;
	AllocationRefCountList				_allRefCountList;
	AllocationRefCountList				_availableRefCountList;
	// clang-format on
};

}
