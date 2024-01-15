#include "CommandListPool.h"
#include "Device.h"
#include "Foundation/StringUtil.h"

namespace dx {

void CommandListPool::OnCreate(Device *pDevice, uint32_t numCmdList, D3D12_COMMAND_LIST_TYPE type) {
    std::string_view name = type == D3D12_COMMAND_LIST_TYPE_COMPUTE ? "ComputeCommandListPool"
                                                                    : "GraphicsCommandListPool";
    ID3D12Device *device = pDevice->GetNativeDevice();

    _cmdList.resize(numCmdList);
    ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&_pCommandAllocator)));
    std::wstring allocatorName = nstd::to_wstring(fmt::format("{}::Allocator", name.data()));
    _pCommandAllocator->SetName(allocatorName.c_str());

    for (uint32_t i = 0; i < numCmdList; ++i) {
        std::wstring cmdListName = nstd::to_wstring(fmt::format("{}::CommandList {}", name.data(), i));
        ThrowIfFailed(
            device->CreateCommandList(0, type, _pCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&_cmdList[i])));
        _cmdList[i]->SetName(cmdListName.c_str());
        ThrowIfFailed(_cmdList[i]->Close());
    }

    ID3D12CommandQueue *pQueue = nullptr;
#if ENABLE_D3D_COMPUTE_QUEUE
    if (type == D3D12_COMMAND_LIST_TYPE_COMPUTE) {
        pQueue = pDevice->GetComputeQueue();
    }
#endif
    if (type == D3D12_COMMAND_LIST_TYPE_DIRECT) {
        pQueue = pDevice->GetGraphicsQueue();
    }

    std::vector<ID3D12CommandList *> cmdList;
    for (size_t i = 0; i < numCmdList; ++i) {
        cmdList.push_back(_cmdList[i].Get());
    }
    pQueue->ExecuteCommandLists(cmdList.size(), cmdList.data());
    pDevice->WaitForGPUFlush();
}

void CommandListPool::OnDestroy() {
    _cmdList.clear();
}

void CommandListPool::OnBeginFrame() {
    ThrowIfFailed(_pCommandAllocator->Reset());
    _commandListIndex = 0;
}

auto CommandListPool::AllocCommandList() -> NativeCommandList * {
    Assert(_commandListIndex < _cmdList.size());
    std::size_t i = _commandListIndex++;
    ThrowIfFailed(_cmdList[i]->Reset(_pCommandAllocator.Get(), nullptr));
    return _cmdList[i].Get();
}

}    // namespace dx
