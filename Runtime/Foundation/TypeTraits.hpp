#pragma once
#include <type_traits>
#include <vector>
#include <list>
#include <forward_list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>

template<typename T>
struct IsStdVector : public std::false_type {};

template<typename T, typename Alloc>
struct IsStdVector<std::vector<T, Alloc>> : public std::true_type {};

template<typename T>
concept StdVectorConcept = IsStdVector<T>::value;

template<typename T>
struct IsStdList : public std::false_type {};

template<typename T, typename Alloc>
struct IsStdList<std::list<T, Alloc>> : public std::true_type {};

template<typename T>
struct IsStdForwardList : public std::false_type {};

template<typename T, typename Alloc>
struct IsStdForwardList<std::forward_list<T, Alloc>> : public std::true_type {};

template<typename T>
struct IsStdSet : public std::false_type {};

template<typename T, typename... Args>
struct IsStdSet<std::set<T, Args...>> : public std::true_type {
	using KeyType = T;
};

template<typename T>
struct IsStdMultiSet : public std::false_type {};

template<typename T, typename... Args>
struct IsStdMultiSet<std::multiset<T, Args...>> : public std::true_type {
	using KeyType = T;
};

template<typename T>
struct IsStdMap : public std::false_type {};

template<typename... Args>
struct IsStdMap<std::map<Args...>> : public std::true_type {};

template<typename T>
struct IsStdMultiMap : public std::false_type {};

template<typename... Args>
struct IsStdMultiMap<std::multimap<Args...>> : public std::true_type {};

template<typename T>
struct IsStdUnorderedSet : public std::false_type {};

template<typename... Args>
struct IsStdUnorderedSet<std::unordered_set<Args...>> : public std::true_type {};

template<typename T>
struct IsStdUnorderedMultiSet : public std::false_type {};

template<typename... Args>
struct IsStdUnorderedMultiSet<std::unordered_multiset<Args...>> : public std::true_type {};

template<typename T>
struct IsStdUnorderedMap : public std::false_type {};

template<typename... Args>
struct IsStdUnorderedMap<std::unordered_map<Args...>> : public std::true_type {};

template<typename T>
struct IsStdUnorderedMultiMap : public std::false_type {};

template<typename... Args>
struct IsStdUnorderedMultiMap<std::unordered_multimap<Args...>> : public std::true_type {};


template<typename T>
struct GetStdMapPair;

template<typename Key, typename Value, typename ...Args>
struct GetStdMapPair<std::map<Key, Value, Args...>> {
	using KeyType = Key;
	using ValueType = Value;
	using PairType = typename std::map<Key, Value, Args...>::value_type;
};

template<typename Key, typename Value, typename ...Args>
struct GetStdMapPair<std::multimap<Key, Value, Args...>> {
	using KeyType = Key;
	using ValueType = Value;
	using PairType = typename std::multimap<Key, Value, Args...>::value_type;
};

template<typename Key, typename Value, typename ...Args>
struct GetStdMapPair<std::unordered_map<Key, Value, Args...>> {
	using KeyType = Key;
	using ValueType = Value;
	using PairType = typename std::unordered_map<Key, Value, Args...>::value_type;
};

template<typename Key, typename Value, typename ...Args>
struct GetStdMapPair<std::unordered_multimap<Key, Value, Args...>> {
	using KeyType = Key;
	using ValueType = Value;
	using PairType = typename std::unordered_multimap<Key, Value, Args...>::value_type;
};

template<typename T>
concept Enumerable = std::is_enum_v<T>;


template<typename T>
consteval auto GetTypeName() -> std::string_view {
    std::string_view name;
#ifdef __clang__
    name = __PRETTY_FUNCTION__;
    auto start = name.find("T = ") + 4; // 4 is length of "T = "
    auto end = name.find_last_of(']');
    return std::string_view{ name.data() + start, end - start };

#elif defined(__GNUC__)
    name = __PRETTY_FUNCTION__;
    auto start = name.find("T = ") + 4; // 4 is length of "T = "
    auto end = name.find_last_of(']');
    return std::string_view{ name.data() + start, end - start };

#elif defined(_MSC_VER)
    name = __FUNCSIG__;
    auto start = name.find("type_name<") + 10; // 10 is length of "type_name<"
    auto end = name.find_last_of('>');
    return std::string_view{ name.data() + start, end - start };
#endif
}