#pragma once
#include "InstanceID.hpp"
#include "TypeBase.hpp"

class Object : public TypeBase {
public:
	DECLARE_CLASS(Object);
	DECLARE_VIRTUAL_SERIALIZER(Object);
public:
	Object();
	void SetInstanceID(InstanceID instanceID) {
		_instanceId = instanceID;
	}
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