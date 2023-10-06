#include "D3dUtils.h"

namespace dx {

class DynamicDescriptorHeap : public NonCopyable {
public:
	DynamicDescriptorHeap(Device *pDevice,
		D3D12_DESCRIPTOR_HEAP_TYPE heapType,
		size_t numDescriptorsPerHeap
	);
	void ParseRootSignature(RootSignature *pRootSignature);
	void Reset();
	void StageDescirptors(size_t rootParameterIndex, 
		size_t offset,
		size_t numDescriptors,
		const D3D12_CPU_DESCRIPTOR_HANDLE &baseDescriptor);
public:
	void CommitStagedDescriptorForDraw(ID3D12GraphicsCommandList6* pCommandList) {
		CommitDescriptorTables(pCommandList, &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable);
	}
	void CommitStagedDescriptorForDispatch(ID3D12GraphicsCommandList6* pCommandList) {
		CommitDescriptorTables(pCommandList, &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable);
	}
private:
	struct DescriptorTableCache;
	using CommitFunc = decltype(&ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable);
	using DescriptorHeapPool = std::queue<WRL::ComPtr<ID3D12DescriptorHeap>>;
	using DescriptorHandleCache = std::array<D3D12_CPU_DESCRIPTOR_HANDLE, kMaxDescriptor>;

	struct DescriptorTableCache {
		size_t                       numDescriptors;
		D3D12_CPU_DESCRIPTOR_HANDLE *pBaseHandle;
	};

	void CommitDescriptorTables(ID3D12GraphicsCommandList6 *pCommandList, CommitFunc commitFunc);
	auto RequestDescriptorHeap() -> WRL::ComPtr<ID3D12DescriptorHeap>;
private:
	// clang-format off
	Device							   *_pDevice;
	size_t								_numDescriptorsPreHeap;
	size_t								_descriptorHandleIncrementSize;
	D3D12_DESCRIPTOR_HEAP_TYPE			_heapType;
	DescriptorTableCache				_descriptorTableCache[kMaxRootParameter];
	std::bitset<kMaxRootParameter>		_descriptorTableBitMask;
	std::bitset<kMaxRootParameter>		_staleDescriptorTableBitMask;
	DescriptorHandleCache				_descriptorHandleCache;
	DescriptorHeapPool                  _descriptorHeapPool;
	DescriptorHeapPool                  _availableDescriptorHeaps;
	size_t								_numFreeHandles;
	CD3DX12_CPU_DESCRIPTOR_HANDLE		_currentCPUDescriptorHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE		_currentGPUDescriptorHandle;
	WRL::ComPtr<ID3D12DescriptorHeap>	_pCurrentDescriptorHeap;
	// clang-format on
};

}