#pragma once
#include <uuid.h>
#include "Serialize/Transfer.hpp"

class UUID128 : public uuids::uuid {
    DECLARE_SERIALIZER(UUID128)
public:
    auto ToString() const -> std::string;
    bool FromString(std::string_view string);
    static auto New() -> UUID128;
    static auto New(std::string_view name) -> UUID128;
    static auto New(std::wstring_view name) -> UUID128;
private:
    UUID128(const uuids::uuid &id);
    static constexpr std::string_view sClassUUID = "19128E59-A779-45B1-8AD9-3F2191D51412";
    static auto GetNameGenerator() -> uuids::uuid_name_generator &;
    static auto GetRandomGenerator() -> uuids::uuid_random_generator &;
};

namespace std {

template<>
struct hash<UUID128> : public hash<uuids::uuid> {};

}    // namespace std