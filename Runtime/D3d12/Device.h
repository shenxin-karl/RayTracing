#pragma once
#include "D3dStd.h"
#include "DescriptorHandle.h"

namespace dx {

class Device : NonCopyable {
public:
    Device();
    ~Device();
public:
    void OnCreate(bool validationEnabled);
    void OnDestroy();
    auto GetNativeDevice() const -> NativeDevice * {
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
#if ENABLE_D3D_COMPUTE_QUEUE
    auto GetComputeQueue() const -> ID3D12CommandQueue * {
        return _pComputeQueue.Get();
    }
#endif
    auto GetAllocator() const -> D3D12MA::Allocator * {
        return _pAllocator;
    }

    auto GetWorkGroupWarpSize() const -> size_t {
        return _workGroupWarpSize;
    }

    void WaitForGPUFlush(D3D12_COMMAND_LIST_TYPE queueType);
    void WaitForGPUFlush();

    template<typename T>
    auto AllocDescriptor(size_t numDescriptor) -> T {
        if constexpr (std::is_same_v<T, RTV>) {
            return static_cast<T>(AllocDescriptorInternal(numDescriptor, D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
        } else if constexpr (std::is_same_v<T, DSV>) {
            return static_cast<T>(AllocDescriptorInternal(numDescriptor, D3D12_DESCRIPTOR_HEAP_TYPE_DSV));
        } else if constexpr (std::is_same_v<T, CBV> || std::is_same_v<T, SRV> || std::is_same_v<T, UAV>) {
            return static_cast<T>(AllocDescriptorInternal(numDescriptor, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
        } else if constexpr (std::is_same_v<T, SAMPLER>) {
            return static_cast<T>(AllocDescriptorInternal(numDescriptor, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER));
        }
        Assert(false);
        return T{};
    }

    void ReleaseStaleDescriptors();
private:
    auto AllocDescriptorInternal(size_t numDescriptor, D3D12_DESCRIPTOR_HEAP_TYPE type) -> DescriptorHandle;
private:
    // clang-format off
    WRL::ComPtr<NativeDevice>           _pDevice ;
    WRL::ComPtr<IDXGIAdapter>           _pAdapter;
    D3D12MA::Allocator                 *_pAllocator;
	std::unique_ptr<DescriptorManager>	_pDescriptorManager;
    WRL::ComPtr<ID3D12CommandQueue>     _pCopyQueue;
    WRL::ComPtr<ID3D12CommandQueue>     _pDirectQueue;
    std::unique_ptr<Fence>              _pCopyQueueFence;
    std::unique_ptr<Fence>              _pDirectQueueFence;
#if ENABLE_D3D_COMPUTE_QUEUE
    WRL::ComPtr<ID3D12CommandQueue>     _pComputeQueue;
    std::unique_ptr<Fence>              _pComputeQueueFence;
#endif
    size_t                              _workGroupWarpSize;
    // clang-format on
};

}    // namespace dx