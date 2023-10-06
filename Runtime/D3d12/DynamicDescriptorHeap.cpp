#include "DynamicDescriptorHeap.h"
#include "Device.h"

namespace dx {

DynamicDescriptorHeap::DynamicDescriptorHeap(Device* pDevice,
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
		_descriptorTableCache[i] = { 0, nullptr };
		_descriptorHandleCache[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT);
	}
}

void DynamicDescriptorHeap::StageDescirptors(size_t rootParameterIndex,
	size_t offset,
	size_t numDescriptors,
	const D3D12_CPU_DESCRIPTOR_HANDLE& baseDescriptor) {

	if (numDescriptors > _numDescriptorsPreHeap || rootParameterIndex >= kMaxRootParameter) {
		Assert("Out of range!");
	}


}

}