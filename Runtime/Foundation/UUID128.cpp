#include "UUID128.h"

IMPLEMENT_SERIALIZER(UUID128)

template<>
struct TransferHelper<uuids::uuid> {
	template<TransferContextConcept Transfer>
    static void Transfer(Transfer &transfer, std::string_view name, uuids::uuid &data) {
		if constexpr (transfer.IsReading()) {
			std::string string;
	        transfer.Transfer(name, string);
	        std::optional<uuids::uuid> pID = uuids::uuid::from_string(string);
	        if (pID != std::nullopt) {
		        data = *pID;
	        }
		} else {
			std::string string = uuids::to_string(data);
			transfer.Transfer(name, string);
		}
	}
};

template<TransferContextConcept T>
void UUID128::TransferImpl(T &transfer) {
	TransferHelper<uuid>::Transfer(transfer, "data", static_cast<uuid &>(*this));
}

auto UUID128::ToString() const -> std::string {
	return uuids::to_string(*this);
}

bool UUID128::FromString(std::string_view string) {
	std::optional<uuid> pResult = from_string(string);
	if (!pResult.has_value()) {
		return false;
	}
	*this = UUID128(pResult.value());
	return true;
}

auto UUID128::New() -> UUID128 {
    return GetRandomGenerator()();
}

auto UUID128::New(std::string_view name) -> UUID128 {
    return GetNameGenerator()(name);
}

auto UUID128::New(std::wstring_view name) -> UUID128 {
    return GetNameGenerator()(name);
}

UUID128::UUID128(const uuids::uuid &id) : uuid(id) {
}

auto UUID128::GetNameGenerator() -> uuids::uuid_name_generator & {
    static uuids::uuid_name_generator generator(from_string(sClassUUID).value());
    return generator;
}

auto UUID128::GetRandomGenerator() -> uuids::uuid_random_generator & {
    static uuids::uuid_random_generator generator = []() {
        auto currentTime = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime.time_since_epoch()).count();
        std::mt19937 seed(timestamp);
        return uuids::uuid_random_generator(seed);
    }();
    return generator;
}
