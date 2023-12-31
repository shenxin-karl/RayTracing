
#pragma once
#include "D3dStd.h"

namespace dx {

class DynamicBufferAllocator : NonCopyable {
public:
    struct AllocInfo {
        uint8_t                      *pBuffer;
        D3D12_GPU_VIRTUAL_ADDRESS     virtualAddress;
    };
public:
    DynamicBufferAllocator();
    ~DynamicBufferAllocator();
public:
    void OnCreate(Device *pDevice);
    void OnDestroy();
    auto AllocVertexBuffer(size_t numOfVertices, size_t strideInBytes, const void *pInitData)
        -> D3D12_VERTEX_BUFFER_VIEW;
    auto AllocIndexBuffer(size_t numOfIndex, size_t strideInBytes, const void *pInitData) -> D3D12_INDEX_BUFFER_VIEW;
    auto AllocConstantBuffer(size_t strideInBytes, const void *pInitData) -> D3D12_GPU_VIRTUAL_ADDRESS;
    auto AllocStructuredBuffer(size_t numOfVertices, size_t strideInBytes, const void *pInitData)
        -> D3D12_GPU_VIRTUAL_ADDRESS;
    auto AllocBuffer(size_t bufferSize, size_t addressAlignment = 1) -> AllocInfo;
    void Reset();
private:
    // clang-format off
    struct MemoryBlock {
        uint8_t                            *pBegin          = nullptr;
        uint8_t                            *pEnd            = nullptr;
        uint8_t                            *pCurrent        = nullptr;
        D3D12_GPU_VIRTUAL_ADDRESS           virtualAddress  = static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(-1);
        WRL::ComPtr<D3D12MA::Allocation>    pAllocation     = nullptr;
    };
    using MemoryBlockList = std::vector<MemoryBlock>;
private:
    Device          *_pDevice;
    size_t           _maxBufferSize;
    MemoryBlockList  _memoryBlockList;
    // clang-format on
};

}    // namespace dx