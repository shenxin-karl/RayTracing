#pragma once
#include <d3d12.h>
#include <vector>
#include <unordered_map>
#include "D3dStd.h"
#include "DescriptorHandleArray.hpp"

namespace dx {

class BindlessCollection : NonCopyable {
public:
    BindlessCollection(size_t maxHandleCount = kDynamicDescriptorMaxView) : _maxHandleCount(maxHandleCount) {
        _pHandleArray = std::make_shared<DescriptorHandleArray>();
        _pHandleArray->Reserve(maxHandleCount);
    }
    void AddHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle) {
        if (handle.ptr == 0) {
	        return;
        }
        Assert(GetCount() <= _maxHandleCount);
        if (_handleMap.contains(handle)) {
            return;
        }
        _pHandleArray->Add(handle);
        _handleMap[handle] = _pHandleArray->Count() - 1;
    }
    auto GetHandleIndex(D3D12_CPU_DESCRIPTOR_HANDLE handle) const -> int {
        if (handle.ptr == 0) {
            return 0;
        }
        auto iter = _handleMap.find(handle);
        if (iter != _handleMap.end()) {
            return static_cast<int>(iter->second);
        }
        return 0;
    }
    auto GetCount() const -> size_t {
        return _pHandleArray->Count();
    }
    auto GetHandles() const -> const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> & {
        return _pHandleArray->GetHandles();
    }
    auto GetHandleArrayPtr() const -> std::shared_ptr<DescriptorHandleArray> {
	    return _pHandleArray;
    }
    bool EnsureCapacity(bool size) const {
	    return _pHandleArray->Count() + size <= _maxHandleCount;
    }
private:
    using HandleArrayPtr = std::shared_ptr<DescriptorHandleArray>;
    using HandleMap = std::unordered_map<D3D12_CPU_DESCRIPTOR_HANDLE, size_t>;
private:
    // clang-format off
	size_t          _maxHandleCount;
	HandleArrayPtr  _pHandleArray;
	HandleMap       _handleMap;
    // clang-format on
};

}    // namespace dx
