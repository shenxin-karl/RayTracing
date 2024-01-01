#pragma once
#include "D3dStd.h"

namespace dx {

class Fence : NonCopyable {
public:
    Fence();
    ~Fence();
public:
    void OnCreate(Device *pDevice, std::string_view name = std::source_location::current().function_name());
    void OnDestroy();
    auto IssueFence(ID3D12CommandQueue *pCommandQueue) -> uint64_t;
    void CpuWaitForFence() const;
    void CpuWaitForFence(uint64_t waitFenceValue) const;
    void GpuWaitForFence(ID3D12CommandQueue *pCommandQueue) const;
    void SetName(std::string_view name);
private:
    // clang-format off
    HANDLE                   _event;
    uint64_t                 _fenceCounter;
    WRL::ComPtr<ID3D12Fence> _pFence;
    // clang-format on
};

}    // namespace dx