#pragma once
#include "D3dStd.h"
#include "Foundation/ReadonlyArraySpan.hpp"

namespace dx {

class DescriptorHandleArray : private NonCopyable {
public:
	constexpr static size_t npos = -1;
	using const_iterator = std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>::const_iterator;
public:
	void Add(D3D12_CPU_DESCRIPTOR_HANDLE handle) {
		_handles.push_back(handle);
	}
	void Add(ReadonlyArraySpan<D3D12_CPU_DESCRIPTOR_HANDLE> handles) {
        _handles.insert(_handles.end(), handles.begin(), handles.end());
	}
	void Erase(D3D12_CPU_DESCRIPTOR_HANDLE handle) {
		std::erase(_handles, handle);
	}
	void Erase(size_t i) {
		Assert(i < _handles.size());
		if (i == _handles.size()-1) {
			_handles.pop_back();
		} else {
			std::swap(_handles[i], _handles.back());
			_handles.pop_back();
		}
	}
	void Clear() {
		_handles.clear();
	}
	void Reserve(size_t newCapacity) {
		_handles.reserve(newCapacity);
	}
	auto Find(D3D12_CPU_DESCRIPTOR_HANDLE handle) const-> size_t {
		for (size_t i = 0; i < _handles.size(); ++i) {
			if (_handles[i] == handle) {
				return i;
			}
		}
		return npos;
	}
	auto Count() const -> size_t {
		return _handles.size();
	}
	auto begin() const -> const_iterator {
		return _handles.begin();
	}
	auto end() const -> const_iterator {
		return _handles.end();
	}
	auto GetHandles() const -> const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> & {
		return _handles;
	}
private:
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> _handles;
};

}
