#pragma once
#include <utility>
#include <cstdint>

constexpr size_t hash_combine(size_t seed, size_t value) {
    return seed ^= value + static_cast<size_t>(0x9e3779b9) + (seed<<6) + (seed>>2);
}

template<typename T>
size_t hash_value(const T &obj) {
	return std::hash<T>{}(obj);
}

template<typename T, typename ...Args>
size_t hash_value(const T &front, const Args&&...args) {
	return hash_combine(hash_value(front), hash_value(args...));
}

template<typename T>
constexpr size_t combine_and_hash_value(size_t seed, const T &obj) {
	return hash_combine(seed, hash_value(obj));
}