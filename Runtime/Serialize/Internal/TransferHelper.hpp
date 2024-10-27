#pragma once
#include "TransferBase.hpp"
#include "Foundation/NamespeceAlias.h"
#include "Foundation/TypeTraits.hpp"

template<typename T>
struct TransferHelper {
    template<TransferContextConcept Archive >
        requires requires { std::declval<Archive &>().Transfer(std::string_view{}, std::declval<T &>()); }
    static void Transfer(Archive &archive, std::string_view name, T &data) {
        archive.Transfer(name, data);
    }
};

template<>
struct TransferHelper<stdfs::path> {
public:
    template<TransferContextConcept Archive>
    static void Transfer(Archive &archive, std::string_view name, stdfs::path &data) {
        if constexpr (archive.IsReading()) {
            std::string string;
            archive.Transfer(name, string);
            data = string;
        } else {
            std::string string = data.string();
            archive.Transfer(name, string);
        }
    }
};

template<Enumerable T>
struct TransferHelper<T> {
    template<TransferContextConcept Archive>
    static void Transfer(Archive &archive, std::string_view name, T &data) {
        using NativeType = std::underlying_type_t<T>;
        archive.transfer(name, static_cast<NativeType &>(data));
    }
};