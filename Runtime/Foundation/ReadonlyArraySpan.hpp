#pragma once
#include "Foundation/Exception.h"
#include <vector>

template<typename T>
class ReadonlyArraySpan final {
public:
    constexpr ReadonlyArraySpan() noexcept : _pData(nullptr), _count(0) {
    }
    constexpr ReadonlyArraySpan(const T *pData, size_t count) noexcept : _pData(pData), _count(count) {
    }
    constexpr ReadonlyArraySpan(const T &value) noexcept : _pData(&value), _count(1) {
    }
    template<std::size_t N>
    constexpr ReadonlyArraySpan(const T (&array)[N]) noexcept : _pData(array), _count(N) {
    }
    constexpr ReadonlyArraySpan(const std::initializer_list<T> &list) noexcept
        : _pData(list.begin()), _count(list.size()) {
    }
    constexpr ReadonlyArraySpan(const std::initializer_list<const T> &list) noexcept
        : _pData(list.begin()), _count(list.size()) {
    }
    template<size_t N>
    constexpr ReadonlyArraySpan(const std::span<T, N> &span) noexcept : _pData(span.data()), _count(span.size()) {
    }
    template<size_t N>
    constexpr ReadonlyArraySpan(const std::span<const T, N> &span) noexcept : _pData(span.data()), _count(span.size()) {
    }
    template<size_t N>
    constexpr ReadonlyArraySpan(const std::array<T, N> &array) noexcept : _pData(array.data()), _count(N) {
    }
    template<size_t N>
    constexpr ReadonlyArraySpan(const std::array<const T, N> &array) noexcept : _pData(array.data()), _count(N) {
    }
    template<typename Allocator>
    constexpr ReadonlyArraySpan(const std::vector<T, Allocator> &vector)
        : _pData(vector.data()), _count(vector.size()) {
    }
    template<typename Allocator>
    constexpr ReadonlyArraySpan(const std::vector<const T, Allocator> &vector)
        : _pData(vector.data()), _count(vector.size()) {
    }
    constexpr auto begin() const noexcept -> const T * {
        return _pData;
    }
    constexpr auto end() const noexcept -> const T * {
        return _pData + _count;
    }
    auto Front() const -> const T & {
        Assert(_pData != nullptr);
        return *_pData;
    }
    auto Back() const -> const T & {
        Assert(_count >= 1);
        return _pData[_count - 1];
    }
    constexpr bool Empty() const noexcept {
        return _pData == nullptr;
    }
    constexpr auto Size() const noexcept -> size_t {
        return _count;
    }
    constexpr auto Count() const noexcept -> size_t {
        return _count;
    }
    constexpr auto Data() const noexcept -> const T * {
        return _pData;
    }
    auto operator[](size_t index) const -> const T & {
        Assert(index < _count);
        return _pData[index];
    }
private:
    // clang-format off
	const T	   *_pData;
	size_t	    _count;
    // clang-format on
};