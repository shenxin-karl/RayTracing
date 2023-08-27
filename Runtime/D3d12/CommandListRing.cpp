#include "CommandListRing.h"
#include "Device.h"
#include "Foundation/StringConvert.h"

namespace dx {

CommandListRing::CommandListRing()
    : _frameIndex(0),
      _numberOfAllocators(0),
      _commandListsPreBackBuffer(0),
      _commandListType(static_cast<D3D12_COMMAND_LIST_TYPE>(-1)) {
}

CommandListRing::~CommandListRing() {
}

void CommandListRing::OnCreate(Device *pDevice,
    uint32_t numberOfBackBuffer,
    uint32_t commandListsPreBackBuffer,
    D3D12_COMMAND_LIST_TYPE type) {

    _fence.OnCreate(pDevice, "CommandListRing Fence");
    _numberOfAllocators = numberOfBackBuffer;
    _commandListsPreBackBuffer = commandListsPreBackBuffer;
    _commandListType = type;
    _frames.clear();
    _frameIndex = 0;

    ID3D12Device *device = pDevice->GetDevice();
    for (uint32_t i = 0; i < _numberOfAllocators; ++i) {
        CommandBuffersPreFrame frame;
        ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&frame.pAllocator)));
        frame.pAllocator->SetName(nstd::to_wstring(fmt::format("CommandAllocator {}", i)).c_str());
        frame.cmdLists.resize(commandListsPreBackBuffer);
        for (uint32_t j = 0; j < commandListsPreBackBuffer; ++j) {
            ThrowIfFailed(
                device->CreateCommandList(0, type, frame.pAllocator.Get(), nullptr, IID_PPV_ARGS(&frame.cmdLists[j])));
            ThrowIfFailed(frame.cmdLists[j]->Close());
            frame.cmdLists[j]->SetName(nstd::to_wstring(fmt::format("CommandList {}, Allocator{}", j, i)).c_str());
        }
        _frames.push_back(std::move(frame));
    }

    ID3D12CommandQueue *pQueue = type == D3D12_COMMAND_LIST_TYPE_COMPUTE ? pDevice->GetComputeQueue()
                                                                         : pDevice->GetGraphicsQueue();
    for (size_t i = 0; i < numberOfBackBuffer; ++i) {
        CommandBuffersPreFrame &frame = _frames[i];
        std::vector<ID3D12CommandList *> cmds;
        for (size_t j = 0; j < commandListsPreBackBuffer; ++j) {
            cmds.push_back(frame.cmdLists[j].Get());
        }
        pQueue->ExecuteCommandLists(cmds.size(), cmds.data());
    }
    pDevice->WaitForGPUFlush();
}

void CommandListRing::OnDestroy() {
    _fence.OnDestroy();
}

void CommandListRing::OnBeginFrame() {
    _frameIndex = (_frameIndex + 1) % _numberOfAllocators;
    _fence.CpuWaitForFence(_frames[_frameIndex].fence);

    ThrowIfFailed(_frames[_frameIndex].pAllocator->Reset());
    _frames[_frameIndex].allocCount = 0;
}

auto CommandListRing::GetNewCommandList() -> ID3D12GraphicsCommandList6 * {
    CommandBuffersPreFrame &frame = _frames[_frameIndex];
    Assert(frame.allocCount < _commandListsPreBackBuffer);
    ID3D12GraphicsCommandList6 *pCmdList = frame.cmdLists[frame.allocCount++].Get();
    ThrowIfFailed(pCmdList->Reset(frame.pAllocator.Get(), nullptr));
    return pCmdList;
}

auto CommandListRing::GetAllocator() const -> ID3D12CommandAllocator * {
    return _frames[_frameIndex].pAllocator.Get();
}

void CommandListRing::OnEndFrame(ID3D12CommandQueue *pCommandQueue) {
    _frames[_frameIndex].fence = _fence.IssueFence(pCommandQueue);
}

}    // namespace dx