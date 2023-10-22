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
    auto GetCpuHandle(size_t offset = 0) const -> D3D12_CPU_DESCRIPTOR_HANDLE;
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

// clang-format off
class RTV       final : public DescriptorHandle {};
class DSV       final : public DescriptorHandle {};
class SAMPLER   final : public DescriptorHandle {};
class CBV       final : public DescriptorHandle {};
class UAV       final : public DescriptorHandle {};
class SRV       final : public DescriptorHandle {};
// clang-format on

}    // namespace dx
