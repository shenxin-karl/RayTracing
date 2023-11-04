#pragma once
#include "D3dUtils.h"

namespace dx {

// acceleration structure builder
class ASBuilder : public NonCopyable {
public:
    ~ASBuilder();
    void OnCreate(Device *pDevice);
    void OnDestroy();
    auto GetScratchBuffer() const -> ID3D12Resource *;
    auto GetInstanceBuffer() const -> std::span<D3D12_RAYTRACING_INSTANCE_DESC>;
    auto GetInstanceBufferGPUAddress() const -> D3D12_GPU_VIRTUAL_ADDRESS;
    auto GetScratchBufferGPUAddress() const -> D3D12_GPU_VIRTUAL_ADDRESS;
    void BuildRayTracingAccelerationStructure(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC &desc,
        ID3D12Resource *pResource);
    void BuildFinish();

    void UpdateScratchBufferSize(size_t bufferSize) {
        _scratchBufferSize = std::max(_scratchBufferSize, bufferSize);
        _scratchBufferSize = AlignUp(_scratchBufferSize, 64);
    }
    void UpdateInstanceBufferElementCount(size_t elementCount) {
        _instanceCount = std::max(_instanceCount, elementCount);
    }
    auto GetDevice() const {
        return _pDevice;
    }
private:
    // clang-format off
    Device                                     *_pDevice                = nullptr;
    WRL::ComPtr<NativeCommandList>              _pCommandList           = nullptr;
    WRL::ComPtr<ID3D12CommandAllocator>         _pCommandAllocator      = nullptr;

    size_t                                      _scratchBufferSize      = 0;
    mutable WRL::ComPtr<D3D12MA::Allocation>    _pScratchBuffer         = nullptr;

    size_t                                      _instanceCount          = 0;
    mutable D3D12_RAYTRACING_INSTANCE_DESC     *_pInstanceBufferAddress = nullptr;
    mutable WRL::ComPtr<D3D12MA::Allocation>    _pInstanceBuffer        = nullptr;
    // clang-format on
};

}    // namespace dx
