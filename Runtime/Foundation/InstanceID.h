#pragma once
#include <compare>
#include <cstdint>
#include "Exception.h"
#include "Serialize/Transfer.hpp"

class InstanceID {
	DECLARE_SERIALIZER(InstanceID);
public:
	explicit InstanceID(uint64_t id) : _id(id) {
		Assert(id < Max());
	}
	InstanceID(const InstanceID &) = default;
	InstanceID(InstanceID &&) = default;
	auto operator=(const InstanceID &) -> InstanceID & = default;
	auto operator=(InstanceID &&) -> InstanceID & = default;
	auto operator<=>(const InstanceID &) const -> std::strong_ordering = default;
	auto GetID() const -> std::uint64_t {
		return _id;
	}
	bool IsValid() const noexcept {
		return static_cast<bool>(*this);
	}
	explicit operator bool() const noexcept {
		return _id < Max();
	}
	static auto Max() noexcept -> uint64_t {
		return 1024 * 1024 * 1024;
	}
	static auto Invalid() noexcept -> InstanceID {
		InstanceID res{0};
		res._id = Max();
		return res;
	}
private:
	uint64_t _id;
};
