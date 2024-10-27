#pragma once
#include <cstdint>
#include "Foundation/TypeSafeWrapper.hpp"
#include "Serialize/Transfer.hpp"

struct InstanceID : public TypeSafeWrapper<int64_t> {
	void Set(int64_t id) {
		_data = id;
	}
	auto Get() const -> int64_t {
		return _data;
	}
	template<typename>
	friend struct TransferHelper;
};

template<>
struct std::hash<InstanceID> : public std::hash<TypeSafeWrapper<int64_t>> {};

template<>
struct TransferHelper<InstanceID> {
    template<TransferContextConcept Archive>
    static void Transfer(Archive &archive, std::string_view name, InstanceID &data) {
        archive.Transfer(name, data._data);
    }
};