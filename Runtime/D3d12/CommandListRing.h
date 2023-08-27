#pragma once
#include <vector>
#include "D3dUtils.h"
#include "Fence.h"

namespace dx {

class CommandListRing : NonCopyable {
public:
    CommandListRing();
    ~CommandListRing();
public:
    void OnCreate(Device *pDevice,
        uint32_t numberOfBackBuffer,
        uint32_t commandListsPreBackBuffer,
        D3D12_COMMAND_LIST_TYPE type);
    void OnDestroy();
    void OnBeginFrame();
    auto GetNewCommandList() -> ID3D12GraphicsCommandList6 *;
    auto GetAllocator() const -> ID3D12CommandAllocator *;
    void OnEndFrame(ID3D12CommandQueue *pCommandQueue);
private:
    struct CommandBuffersPreFrame {
        uint64_t fence = 0;
        uint32_t allocCount = 0;
        WRL::ComPtr<ID3D12CommandAllocator> pAllocator;
        std::vector<WRL::ComPtr<ID3D12GraphicsCommandList6>> cmdLists;
    };
private:
    Fence _fence;
    uint32_t _frameIndex;
    uint32_t _numberOfAllocators;
    uint32_t _commandListsPreBackBuffer;
    D3D12_COMMAND_LIST_TYPE _commandListType;
    std::vector<CommandBuffersPreFrame> _frames;
};

}    // namespace dx
