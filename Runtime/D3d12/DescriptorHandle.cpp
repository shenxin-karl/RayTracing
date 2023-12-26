#include "DescriptorHandle.h"
#include "DescriptorPage.h"

namespace dx {

DescriptorHandle::DescriptorHandle()
    : _numHandle(0), _handleSize(0), _pPage(nullptr), _pRefCount(nullptr), _baseHandle() {
}

DescriptorHandle::DescriptorHandle(const DescriptorHandle &other) : DescriptorHandle() {
    if (other._pRefCount == nullptr) {
        return;
    }

    size_t count = other._pRefCount->load();
    while (count != 0) {
        if (other._pRefCount->compare_exchange_strong(count, count + 1)) {
            break;
        }
    }

    if (count != 0) {
        _numHandle = other._numHandle;
        _handleSize = other._handleSize;
        _pPage = other._pPage;
        _pRefCount = other._pRefCount;
        _baseHandle = other._baseHandle;
    }
}

DescriptorHandle::DescriptorHandle(DescriptorHandle &&other) noexcept {
    _numHandle = std::exchange(other._numHandle, 0);
    _handleSize = std::exchange(other._handleSize, 0);
    _pPage = std::exchange(other._pPage, nullptr);
    _pRefCount = std::exchange(other._pRefCount, nullptr);
    _baseHandle = std::exchange(other._baseHandle, {});
}

DescriptorHandle &DescriptorHandle::operator=(const DescriptorHandle &other) {
    DescriptorHandle tmp{other};
    swap(*this, tmp);
    return *this;
}

DescriptorHandle &DescriptorHandle::operator=(DescriptorHandle &&other) noexcept {
    DescriptorHandle tmp{std::move(other)};
    swap(*this, tmp);
    return *this;
}

DescriptorHandle::~DescriptorHandle() {
    Release();
}

void swap(DescriptorHandle &lhs, DescriptorHandle &rhs) noexcept {
    using std::swap;
    swap(lhs._numHandle, rhs._numHandle);
    swap(lhs._handleSize, rhs._handleSize);
    swap(lhs._pPage, rhs._pPage);
    swap(lhs._pRefCount, rhs._pRefCount);
    swap(lhs._baseHandle, rhs._baseHandle);
}

DescriptorHandle::DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle,
    size_t numHandle,
    size_t handleSize,
    std::atomic_size_t *pRefCount,
    DescriptorPage *pPage) {

    _numHandle = numHandle;
    _handleSize = handleSize;
    _pPage = pPage;
    _pRefCount = pRefCount;
    _baseHandle = handle;
    Assert(pRefCount->load() == 1);
}

void DescriptorHandle::Release() {
    if (_pRefCount == nullptr) {
        return;
    }

    size_t refCount = _pRefCount->fetch_sub(1);
    Assert(refCount >= 1);
    if (refCount == 1) {
        _pPage->Free(_baseHandle, _numHandle, _pRefCount);
    }
    _numHandle = 0;
    _handleSize = 0;
    _pPage = nullptr;
    _pRefCount = nullptr;
    _baseHandle = {};
}

}    // namespace dx
