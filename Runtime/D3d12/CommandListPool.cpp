#include "CommandListPool.h"
#include "Device.h"
#include "Foundation/StringUtil.h"

namespace dx {

void CommandListPool::OnCreate(Device *pDevice, uint32_t numCmdList, D3D12_COMMAND_LIST_TYPE type) {
    std::string_view name = type == D3D12_COMMAND_LIST_TYPE_COMPUTE ? "ComputeCommandListPool" : "GraphicsCommandListPool";
    ID3D12Device *device = pDevice->GetDevice();
    ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&_pAllocator)));
    _pAllocator->SetName(nstd::to_wstring(fmt::format("{}::Allocator", name.data())).c_str());

    _cmdList.resize(numCmdList);
    for (uint32_t i = 0; i < numCmdList; ++i) {
        ThrowIfFailed(device->CreateCommandList(0, type, _pAllocator.Get(), nullptr, IID_PPV_ARGS(&_cmdList[i])));
        ThrowIfFailed(_cmdList[i]->Close());
        _cmdList[i]->SetName(nstd::to_wstring(fmt::format("{}::CommandList {}", name.data(), i)).c_str());
    }

    std::vector<ID3D12CommandList *> cmds;
    ID3D12CommandQueue *pQueue = type == D3D12_COMMAND_LIST_TYPE_COMPUTE ? pDevice->GetComputeQueue()
                                                                         : pDevice->GetGraphicsQueue();
    for (size_t j = 0; j < numCmdList; ++j) {
        cmds.push_back(_cmdList[j].Get());
    }
    pQueue->ExecuteCommandLists(cmds.size(), cmds.data());
    pDevice->WaitForGPUFlush();
}

void CommandListPool::OnDestroy() {
    _cmdList.clear();
    _pAllocator = nullptr;
}

void CommandListPool::OnBeginFrame() {
    _allocCount = 0;
}

auto CommandListPool::AllocCommandList() -> ID3D12GraphicsCommandList6 * {
    Assert(_allocCount < _cmdList.size());
	ID3D12GraphicsCommandList6 *cmd = _cmdList[_allocCount++].Get();
    ThrowIfFailed(cmd->Reset(_pAllocator.Get(), nullptr));
    return cmd;
}

}    // namespace dx
