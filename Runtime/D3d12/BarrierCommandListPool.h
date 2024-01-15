#pragma once
#include <vector>
#include "CommandListPool.h"
#include "Foundation/NonCopyable.h"

namespace dx {

class BarrierCommandListPool : private NonCopyable {
public:
	BarrierCommandListPool();
	~BarrierCommandListPool();
	void OnCreate(Device *pDevice);
	void OnDestroy();
	void OnBeginFrame();
	auto AllocGraphicsCommandList() -> NativeCommandList *;
#if ENABLE_D3D_COMPUTE_QUEUE
	auto AllocComputeCommandList() -> NativeCommandList *;
#endif
private:
	// clang-format off
	Device									   *_pDevice;
	WRL::ComPtr<ID3D12CommandAllocator>			_pGraphicsCommandAllocator;
	std::vector<WRL::ComPtr<NativeCommandList>> _graphicsCommandListPool;
	size_t										_currentGraphicsCommandListIndex;

#if ENABLE_D3D_COMPUTE_QUEUE
	WRL::ComPtr<ID3D12CommandAllocator>			_pComputeCommandAllocator;
	std::vector<WRL::ComPtr<NativeCommandList>> _computeCommandListPool;
	size_t										_currentComputeCommandListIndex;
#endif
	// clang-format on
};

}