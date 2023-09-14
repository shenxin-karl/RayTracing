#pragma once
#include "D3dUtils.h"

namespace dx {

class Fence : NonCopyable {
public:
    Fence();
    ~Fence();
public:
    void OnCreate(Device *pDevice, std::string_view name);
    void OnDestroy();
    auto IssueFence(ID3D12CommandQueue *pCommandQueue) -> uint64_t;
    void CpuWaitForFence(uint64_t olderFence);
    void GpuWaitForFence(ID3D12CommandQueue *pCommandQueue);
private:
    // clang-format off
    HANDLE                   _event;
    uint64_t                 _fenceCounter;
    WRL::ComPtr<ID3D12Fence> _pFence;
    // clang-format on
};

}    // namespace dx