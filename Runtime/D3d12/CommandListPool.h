#pragma once
#include "D3dStd.h"

namespace dx {

class CommandListPool : NonCopyable {
public:
	void OnCreate(Device *pDevice, uint32_t numCmdList, D3D12_COMMAND_LIST_TYPE type);
	void OnDestroy();
	void OnBeginFrame();
	auto AllocCommandList() -> NativeCommandList *;
private:
	using CommandListContainer = std::vector<WRL::ComPtr<NativeCommandList>>;
	using CommandAllocatorContainer = std::vector<WRL::ComPtr<ID3D12CommandAllocator>>;
private:
	// clang-format off
    uint32_t					_allocCount = 0;
	CommandListContainer		_cmdList;
    CommandAllocatorContainer	_allocatorList;
	// clang-format on
};

}
