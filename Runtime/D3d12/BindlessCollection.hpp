#pragma once
#include <d3d12.h>
#include <vector>
#include <unordered_map>
#include "D3dUtils.h"

namespace dx {

class BindlessCollection : NonCopyable {
public:
    BindlessCollection(size_t maxHandleCount = kDynamicDescriptorMaxView) : _maxHandleCount(maxHandleCount) {
        _handles.reserve(maxHandleCount);
    }
    void AddHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle) {
        if (handle.ptr == 0) {
	        return;
        }
        Assert(GetCount() <= _maxHandleCount);
        if (_handleMap.contains(handle)) {
            return;
        }
        _handles.push_back(handle);
        _handleMap[handle] = _handles.size() - 1;
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
        return _handles.size();
    }
    auto GetHandles() const -> const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> & {
        return _handles;
    }
    bool EnsureCapacity(bool size) const {
	    return _handles.size() + size <= _maxHandleCount;
    }
private:
    using HandleList = std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>;
    using HandleMap = std::unordered_map<D3D12_CPU_DESCRIPTOR_HANDLE, size_t>;
private:
    // clang-format off
	size_t     _maxHandleCount;
	HandleList _handles;
	HandleMap  _handleMap;
    // clang-format on
};

}    // namespace dx
