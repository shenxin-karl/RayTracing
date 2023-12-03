#pragma once
#include "DescriptorAllocator.h"
#include "Foundation/NonCopyable.h"

namespace dx {

class DescriptorManager : NonCopyable {
private:
    friend class Device;

    void OnCreate(Device *pDevice) {
        _descriptorAllocatorList.resize(4);

        _descriptorAllocatorList[0] = std::make_unique<DescriptorAllocator>(pDevice,
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            256);

        _descriptorAllocatorList[1] = std::make_unique<DescriptorAllocator>(pDevice,
            D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
            32);

        _descriptorAllocatorList[2] = std::make_unique<DescriptorAllocator>(pDevice,
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            32);

        _descriptorAllocatorList[3] = std::make_unique<DescriptorAllocator>(pDevice,
            D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
            32);
    }

    void OnDestroy() {
        _descriptorAllocatorList.clear();
    }

    void ReleaseStaleDescriptors() {
        for (std::unique_ptr<DescriptorAllocator> &allocator : _descriptorAllocatorList) {
            allocator->ReleaseStateDescriptors();
        }
    }

    auto Alloc(size_t numDescriptor, D3D12_DESCRIPTOR_HEAP_TYPE type) -> DescriptorHandle {
        return _descriptorAllocatorList[type]->Alloc(numDescriptor);
    }
private:
    std::vector<std::unique_ptr<DescriptorAllocator>> _descriptorAllocatorList;
};

}    // namespace dx
