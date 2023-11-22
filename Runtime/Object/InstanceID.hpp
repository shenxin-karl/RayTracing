#pragma once
#include <cstdint>
#include "Serialize/Transfer.hpp"

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
    friend std::strong_ordering operator<=>(const InstanceID &, const InstanceID &) = default;
private:
    friend struct TransferHelper<InstanceID>;
    int64_t _instanceId = 0;
};

template<>
struct TransferHelper<InstanceID> {
    template<TransferContextConcept Transfer>
    static void Transfer(Transfer &transfer, std::string_view name, InstanceID &data) {
        transfer.Transfer(name, data._instanceId);
    }
};

template<>
struct std::hash<InstanceID> {
	typedef InstanceID argument_type;
	typedef std::size_t result_type;

	result_type operator()(const argument_type &s) const noexcept {
		return std::hash<int64_t>{}(s.Get());
	}
};  