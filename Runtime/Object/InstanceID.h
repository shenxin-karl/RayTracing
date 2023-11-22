#pragma once
#include <cstdint>


struct InstanceID {
public:
	void Set(int64_t id) {
		_instanceId = id;
	}
	auto Get() const -> int64_t {
		return _instanceId;
	}
	explicit operator int64_t() const {
		return Get();
	}
private:
	int64_t  _instanceId = 0;
};