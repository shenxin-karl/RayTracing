#include "DescriptorAllocator.h"
#include "DescriptorPage.h"
#include <algorithm>

namespace dx {

DescriptorAllocator::DescriptorAllocator(Device *pDevice,
    D3D12_DESCRIPTOR_HEAP_TYPE heapType,
    size_t numDescriptorPrePage)
    : _pDevice(pDevice), _numDescriptorPrePage(numDescriptorPrePage), _heapType(heapType) {
}

DescriptorAllocator::~DescriptorAllocator() {
    ReleaseStateDescriptors();
}

void DescriptorAllocator::ReleaseStateDescriptors() {
    std::unique_lock lock(_allocMutex);
    for (std::unique_ptr<DescriptorPage> &pPage : _heapPool) {
        pPage->ReleaseStaleDescriptors();
    }
}

auto DescriptorAllocator::Alloc(size_t numDescriptor) -> DescriptorHandle {
    Assert(numDescriptor <= _numDescriptorPrePage);
    std::unique_lock lock(_allocMutex);
    for (std::unique_ptr<DescriptorPage> &pPage : _heapPool) {
        std::optional<DescriptorHandle> pHandle = pPage->Alloc(numDescriptor);
        if (pHandle.has_value()) {
            return pHandle.value();
        }
    }

    lock.unlock();
    std::unique_ptr<DescriptorPage> pPage = std::make_unique<DescriptorPage>(_pDevice,
        _heapType,
        _numDescriptorPrePage);

    lock.lock();
    _heapPool.push_back(std::move(pPage));
    return _heapPool.back()->Alloc(numDescriptor).value();
}

}    // namespace dx
