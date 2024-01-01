#pragma once
#include <mutex>
#include "D3dStd.h"
#include "DescriptorHandle.h"

namespace dx {

class DescriptorAllocator : NonCopyable {
public:
    DescriptorAllocator(Device *pDevice, D3D12_DESCRIPTOR_HEAP_TYPE heapType, size_t numDescriptorPrePage);
    ~DescriptorAllocator();
public:
    void ReleaseStateDescriptors();
    auto Alloc(size_t numDescriptor = 1) -> DescriptorHandle;
private:
    using DescriptorHeapPool = std::vector<std::unique_ptr<DescriptorPage>>;
    struct PageFreeHandle {
        size_t index = 0;
        size_t handleCount = 0;
    };
private:
    // clang-format off
    Device *                    _pDevice;
    size_t                      _numDescriptorPrePage;
    std::mutex                  _allocMutex;
    D3D12_DESCRIPTOR_HEAP_TYPE  _heapType;
    DescriptorHeapPool          _heapPool;
    // clang-format on
};

}    // namespace dx
