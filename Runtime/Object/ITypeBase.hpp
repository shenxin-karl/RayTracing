#pragma once
#include "Foundation/NonCopyable.h"
#include <string_view>
#include <typeindex>
#include "Foundation/TypeSafeWrapper.hpp"

using TypeID = TypeSafeWrapper<std::uint64_t>;

class ITypeBase : private NonCopyable {
public:
    using SuperType = void;
    using ThisType = ITypeBase;
    virtual ~ITypeBase() = default;
    virtual auto GetClassTypeID() const -> TypeID;
    virtual auto GetClassTypeName() const -> std::string_view;
    virtual auto GetClassTypeIndex() const -> std::type_index;
};

namespace TypeIDDetail {

// See http://www.isthe.com/chongo/tech/comp/fnv/#FNV-param
constexpr std::uint64_t fnv_basis = 14695981039346656037ull;
constexpr std::uint64_t fnv_prime = 1099511628211ull;

// FNV-1a 64 bit hash
constexpr std::uint64_t fnv1a_hash(std::size_t n, const char *str, std::uint64_t hash = fnv_basis) {
    return n > 0 ? fnv1a_hash(n - 1, str + 1, (hash ^ *str) * fnv_prime) : hash;
}

constexpr std::uint64_t fnv1a_hash(std::string_view sv) {
    return fnv1a_hash(sv.length(), sv.data());
}

}    // namespace TypeIDDetail

template<typename T>
consteval auto GetTypeName() -> std::string_view {
    std::string_view name;
#ifdef __clang__
    name = __PRETTY_FUNCTION__;
    auto start = name.find("T = ") + 4;    // 4 is length of "T = "
    auto end = name.find_last_of(']');
    return std::string_view{name.data() + start, end - start};

#elif defined(__GNUC__)
    name = __PRETTY_FUNCTION__;
    auto start = name.find("T = ") + 4;    // 4 is length of "T = "
    auto end = name.find_last_of(']');
    return std::string_view{name.data() + start, end - start};

#elif defined(_MSC_VER)
    name = __FUNCSIG__;
    auto start = name.find("GetTypeName<") + 12;    // 10 is length of "GetTypeName<"
    auto end = name.find_last_of('>');
    return std::string_view{name.data() + start, end - start};
#endif
}

template<typename T>
auto GetTypeIndex() -> std::type_index {
    return std::type_index(typeid(T));
}

inline auto GetTypeIndex(const ITypeBase *pObject) -> std::type_index {
    return pObject->GetClassTypeIndex();
}

template<typename T>
consteval auto GetTypeID() -> TypeID {
    return TypeID(TypeIDDetail::fnv1a_hash(GetTypeName<T>()));
}

constexpr auto GetTypeID(std::string_view typeName) -> TypeID {
    return TypeID(TypeIDDetail::fnv1a_hash(typeName));
}

inline auto GetTypeID(const ITypeBase *pObject) -> TypeID {
    return pObject->GetClassTypeID();
}

inline auto ITypeBase::GetClassTypeID() const -> TypeID {
    return ::GetTypeID<ITypeBase>();
}

inline auto ITypeBase::GetClassTypeName() const -> std::string_view {
    return ::GetTypeName<ITypeBase>();
}

inline auto ITypeBase::GetClassTypeIndex() const -> std::type_index {
    return std::type_index(typeid(ITypeBase));
}

#define DECLARE_CLASS(Type)                                                                                            \
public:                                                                                                                \
    using SuperType = ThisType;                                                                                        \
    using ThisType = Type;                                                                                             \
    auto GetClassTypeID() const->TypeID override {                                                                     \
        return ::GetTypeID<Type>();                                                                                    \
    }                                                                                                                  \
    auto GetClassTypeName() const->std::string_view override {                                                         \
        return ::GetTypeName<Type>();                                                                                  \
    }                                                                                                                  \
    auto GetClassTypeIndex() const->std::type_index override {                                                         \
        return std::type_index(typeid(Type));                                                                          \
    }
