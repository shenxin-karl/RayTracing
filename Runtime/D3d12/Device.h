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
    auto GetComputeQueue() const -> ID3D12CommandQueue * {
        return _pComputeQueue.Get();
    }
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
    WRL::ComPtr<ID3D12CommandQueue> _pComputeQueue = nullptr;
    D3D12MA::Allocator *_pAllocator = nullptr;
};

}    // namespace dx