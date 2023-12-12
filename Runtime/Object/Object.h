#pragma once
#include "InstanceID.hpp"
#include "ITypeBase.hpp"
#include "Foundation/Memory/RefCounter.h"

class Object : public ITypeBase, public RefCounter {
public:
	DECLARE_CLASS(Object);
	DECLARE_VIRTUAL_SERIALIZER(Object);
public:
	Object();
	void InitInstanceId();
	void SetName(std::string name) {
		_name = std::move(name);
	}
	auto GetInstanceID() const -> InstanceID {
		return _instanceId;
	}
	auto GetName() const -> const std::string & {
		return _name;
	}
protected:
	// clang-format off
	std::string _name;
	InstanceID  _instanceId;
	// clang-format on
};