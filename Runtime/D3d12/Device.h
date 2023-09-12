#pragma once
#include "D3dUtils.h"

namespace dx {

class Device : public NonCopyable {
public:
    void OnCreate(bool validationEnabled);
    void OnDestroy();
    auto GetDevice() const -> ID3D12Device * {
        return _pDevice.Get();
    }
    auto GetAdapter() const -> IDXGIAdapter * {
        return _pAdapter.Get();
    }
    auto GetCopyQueue() const -> ID3D12CommandQueue * {
        return _pCopyQueue.Get();
    }
    auto GetGraphicsQueue() const -> ID3D12CommandQueue * {
        return _pDirectQueue.Get();
    }
#if ENALBE_D3D_COMPUTE_QUEUE
    auto GetComputeQueue() const -> ID3D12CommandQueue * {
        return _pComputeQueue.Get();
    }
#endif
    auto GetAllocator() const -> D3D12MA::Allocator * {
        return _pAllocator;
    }
    void WaitForGPUFlush(D3D12_COMMAND_LIST_TYPE queueType);
    void WaitForGPUFlush();
private:
    WRL::ComPtr<ID3D12Device> _pDevice = nullptr;
    WRL::ComPtr<IDXGIAdapter> _pAdapter = nullptr;
    WRL::ComPtr<ID3D12CommandQueue> _pCopyQueue = nullptr;
    WRL::ComPtr<ID3D12CommandQueue> _pDirectQueue = nullptr;
#if ENALBE_D3D_COMPUTE_QUEUE
    WRL::ComPtr<ID3D12CommandQueue> _pComputeQueue = nullptr;
#endif
    D3D12MA::Allocator *_pAllocator = nullptr;
};

}    // namespace dx