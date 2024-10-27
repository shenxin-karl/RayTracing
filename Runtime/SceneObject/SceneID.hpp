#pragma once
#include "Foundation/TypeSafeWrapper.hpp"
#include "Serialize/Transfer.hpp"

struct SceneID : public TypeSafeWrapper<int32_t> {
    using TypeSafeWrapper::TypeSafeWrapper;
    constexpr bool IsNull() const {
        return _data == -1;
    }
    constexpr bool IsValid() const {
        return !IsNull();
    }
    constexpr explicit operator bool() const {
        return IsValid();
    }

    template<typename>
	friend struct TransferHelper;

    static SceneID Invalid;
};

inline SceneID SceneID::Invalid{-1};

template<>
struct std::hash<SceneID> : public std::hash<TypeSafeWrapper<int32_t>> {};

template<>
struct TransferHelper<SceneID> {
    template<TransferContextConcept Archive>
    static void Transfer(Archive &archive, std::string_view name, SceneID &data) {
        archive.Transfer(name, data._data);
    }
};