#pragma once
#include "D3dUtils.h"

namespace dx {

// acceleration structure upload heap
class ASUploadHeap : public NonCopyable {
public:
    void OnCreate(Device *pDevice);
    void OnDestroy();
    void UpdateScratchBufferSize(size_t sizeInBytes);
    auto GetScratchBuffer() const -> ID3D12Resource *;
    void BuildRayTracingAccelerationStructure(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC &desc,
        ID3D12Resource *pResource);
    void Finish();
private:
    // clang-format off
    Device                                     *_pDevice            = nullptr;
    size_t                                      _bufferSize         = 0;
    WRL::ComPtr<NativeCommandList>              _pCommandList       = nullptr;
    WRL::ComPtr<ID3D12CommandAllocator>         _pCommandAllocator  = nullptr;
    mutable WRL::ComPtr<D3D12MA::Allocation>    _pScratchBuffer     = nullptr;
    // clang-format on
};

}    // namespace dx
