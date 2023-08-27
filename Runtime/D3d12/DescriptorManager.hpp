#pragma once
#include "DescriptorAllocator.h"
#include "Foundation/Singleton.hpp"

namespace dx {

class DescriptorManager : public Singleton<DescriptorManager> {
public:
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

    template<typename T>
    static auto Alloc(size_t numDescriptor = 1) -> T {
        DescriptorManager *inst = GetInstance();
        if constexpr (std::is_same_v<T, RTV>) {
            return inst->_descriptorAllocatorList[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->Alloc<T>(numDescriptor);
        } else if constexpr (std::is_same_v<T, DSV>) {
            return inst->_descriptorAllocatorList[D3D12_DESCRIPTOR_HEAP_TYPE_DSV]->Alloc<T>(numDescriptor);
        } else if constexpr (std::is_same_v<T, CBV> || std::is_same_v<T, SRV> || std::is_same_v<T, UAV>) {
            return inst->_descriptorAllocatorList[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->Alloc<T>(numDescriptor);
        } else if constexpr (std::is_same_v<T, SAMPLER>) {
            return inst->_descriptorAllocatorList[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER]->Alloc<T>(numDescriptor);
        }
        Assert(false);
        return T{};
    }
private:
    std::vector<std::unique_ptr<DescriptorAllocator>> _descriptorAllocatorList;
};

}    // namespace dx
