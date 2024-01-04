#pragma once
#include "D3dStd.h"
#include "Foundation/ReadonlyArraySpan.hpp"

namespace dx {

class DynamicDescriptorHeap : NonCopyable {
public:
    DynamicDescriptorHeap(Device *pDevice, D3D12_DESCRIPTOR_HEAP_TYPE heapType, size_t numDescriptorsPerHeap);
    void ParseRootSignature(const RootSignature *pRootSignature);
    void Reset();
    void StageDescriptors(size_t rootParameterIndex,
        size_t numDescriptors,
        const D3D12_CPU_DESCRIPTOR_HANDLE &baseDescriptor,
        size_t offset = 0);
    void StageDescriptors(size_t rootParameterIndex, ReadonlyArraySpan<D3D12_CPU_DESCRIPTOR_HANDLE> handles, size_t offset = 0);
    void CommitStagedDescriptorForDraw(NativeCommandList *pCommandList);
    void CommitStagedDescriptorForDispatch(NativeCommandList *pCommandList);
	void EnsureCapacity(NativeCommandList *pCommandList, size_t numDescriptorToCommit);
	auto CommitDescriptorHandleArray(const DescriptorHandleArray *pHandleArray) -> D3D12_GPU_DESCRIPTOR_HANDLE;
    auto ComputeStaleDescriptorCount() const -> size_t;
private:
	struct DescriptorTableCache {
	    void Reset(bool enableBindless, size_t capacity);
	    void Fill(size_t offset, ReadonlyArraySpan<D3D12_CPU_DESCRIPTOR_HANDLE> handles);
	public:
		using HandleContainer = std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>;
		// clang-format off
	    size_t			count;
	    size_t			capacity;
	    bool			enableBindless;
	    HandleContainer	cachedHandles;
		// clang-format on
	};

    using CommitFunc = decltype(&ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable);
    using DescriptorHeapPool = std::queue<WRL::ComPtr<ID3D12DescriptorHeap>>;
    void CommitDescriptorTables(NativeCommandList *pCommandList, CommitFunc commitFunc);
    auto RequestDescriptorHeap() -> WRL::ComPtr<ID3D12DescriptorHeap>;
private:
    // clang-format off
	Device							   *_pDevice;
	const RootSignature				   *_pRootSignature;
	size_t								_numDescriptorsPreHeap;
	size_t								_descriptorHandleIncrementSize;
	D3D12_DESCRIPTOR_HEAP_TYPE			_heapType;
	DescriptorTableCache				_descriptorTableCache[kMaxRootParameter];
	std::bitset<kMaxRootParameter>		_staleDescriptorTableBitMask;
	DescriptorHeapPool                  _descriptorHeapPool;
	DescriptorHeapPool                  _availableDescriptorHeaps;
	size_t								_numFreeHandles;
	CD3DX12_CPU_DESCRIPTOR_HANDLE		_currentCPUDescriptorHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE		_currentGPUDescriptorHandle;
	WRL::ComPtr<ID3D12DescriptorHeap>	_pCurrentDescriptorHeap;
    // clang-format on
};



}    // namespace dx