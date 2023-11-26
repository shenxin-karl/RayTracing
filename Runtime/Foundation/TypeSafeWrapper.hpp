#pragma once
#include <type_traits>
#include <compare>

template<typename T>
    requires(std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_pointer_v<T> || std::is_enum_v<T>)
class TypeSafeWrapper {
public:
    constexpr explicit TypeSafeWrapper(const T &data = T()) : _data(data) {
    }
    constexpr ~TypeSafeWrapper() = default;
    constexpr TypeSafeWrapper(const TypeSafeWrapper &) noexcept = default;
    constexpr TypeSafeWrapper(TypeSafeWrapper &&) noexcept = default;
    constexpr TypeSafeWrapper &operator=(const TypeSafeWrapper &) noexcept = default;
    constexpr TypeSafeWrapper &operator=(TypeSafeWrapper &&) noexcept = default;
    constexpr friend std::strong_ordering operator<=>(const TypeSafeWrapper &,
        const TypeSafeWrapper &) noexcept = default;
    constexpr friend void swap(TypeSafeWrapper &lhs, TypeSafeWrapper &rhs) noexcept {
        using std::swap;
        swap(lhs._data, rhs._data);
    }
    constexpr operator T() const {
        return _data;
    }
protected:
    T _data;
};

template<typename T>
struct std::hash<TypeSafeWrapper<T>> {
    using argument_type = TypeSafeWrapper<T>;
    using result_type = std::size_t;

    [[nodiscard]]
    result_type
    operator()(const argument_type &wrapper) const {
        return std::hash<T>{}(static_cast<T>(wrapper));
    }
};
