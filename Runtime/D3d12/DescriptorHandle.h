#pragma once
#include <atomic>
#include "D3dStd.h"
#include "Foundation/ReadonlyArraySpan.hpp"

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
    auto GetCpuHandle(size_t offset) const -> D3D12_CPU_DESCRIPTOR_HANDLE {
        Assert(offset < _numHandle);
        CD3DX12_CPU_DESCRIPTOR_HANDLE handle(_baseHandle);
        handle.Offset(static_cast<INT>(offset), static_cast<UINT>(_handleSize));
        return handle;
    }
    auto GetCpuHandle() const -> D3D12_CPU_DESCRIPTOR_HANDLE {
        return _baseHandle;
    }
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
    auto operator[](size_t index) const -> D3D12_CPU_DESCRIPTOR_HANDLE {
        return GetCpuHandle(index);
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