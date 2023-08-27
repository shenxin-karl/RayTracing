#pragma once
#include <atomic>
#include "D3dUtils.h"

namespace dx {

class DescriptorHandle {
public:
    DescriptorHandle();
    DescriptorHandle(const DescriptorHandle &other);
    DescriptorHandle(DescriptorHandle &&other) noexcept;
    DescriptorHandle &operator=(const DescriptorHandle &other);
    DescriptorHandle &operator=(DescriptorHandle &&other) noexcept;
    ~DescriptorHandle();
    friend void swap(DescriptorHandle &lhs, DescriptorHandle &rhs) noexcept;
    DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle,
        size_t numHandle,
        size_t handleSize,
        std::atomic_size_t *pRefCount,
        DescriptorPage *pPage);
public:
    void Release();
    auto GetCpuHandle(size_t offset) const -> D3D12_CPU_DESCRIPTOR_HANDLE;
    auto GetNumHandle() const -> size_t {
        return _numHandle;
    }
    bool IsNull() const {
        return _baseHandle.ptr == 0;
    }
    bool IsValid() const {
        return _baseHandle.ptr != 0;
    }
    explicit operator bool() const {
        return IsValid();
    }
private:
    // clang-format off
	uint32_t					_numHandle;
	uint32_t					_handleSize;
	DescriptorPage *			_pPage;
	std::atomic_size_t *		_pRefCount;
	D3D12_CPU_DESCRIPTOR_HANDLE _baseHandle;
    // clang-format on
};

class RTV : public DescriptorHandle {};
class DSV : public DescriptorHandle {};
class SAMPLER : public DescriptorHandle {};
class CBV_SRV_UAV : public DescriptorHandle {};
class CBV : public CBV_SRV_UAV {};
class UAV : public CBV_SRV_UAV {};
class SRV : public CBV_SRV_UAV {};

}    // namespace dx
