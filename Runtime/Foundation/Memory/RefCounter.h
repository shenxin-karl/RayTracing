#pragma once
#include <cstdint>
#include "Foundation/NonCopyable.h"

class RefCounter : private NonCopyable {
public:
	RefCounter() : _refCount(0) {}
	virtual ~RefCounter() = default;
	void Release();
	void AddRef() {
		++_refCount;
	}
	auto GetRefCount() const noexcept -> int32_t {
		return _refCount;
	}
private:
	int32_t	_refCount;
};
