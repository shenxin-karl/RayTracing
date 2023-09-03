#pragma once
#include <type_traits>

#ifdef __IOS__
//todo ::
#elif __ANDROID__
    //android define
    #define inline_0 __attribute__((unused, noinline))
    #define inline_1 __attribute__((unused)) inline
    #define inline_2 __attribute__((unused, always_inline)) inline
    #define inline_3
    #define DLL_Export __attribute((visibility("default")))
    #define DLL_Import
    #define AlignPrefix(length)
    #define AlignSuffix(length) __attribute__((aligned(length)))
    #define stdcall
#elif __GNUC__
    #define MIN_GCC_VERSION 40100
    #define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
    #define inline_0 __attribute__((unused, noinline))
    #define inline_1 __attribute__((unused)) inline
    #if defined(__clang__) || defined(__APPLE_CC__) || defined(__APPLE_CPP__)
        #define inline_2 __attribute__((unused, always_inline, nodebug)) inline
    #else
        #define inline_2 __attribute__((unused, always_inline)) inline
    #endif
    #define inline_3
    #define DLL_Export __attribute((visibility("default")))
    #define DLL_Import
    #define AlignPrefix(length)
    #define AlignSuffix(length) __attribute__((aligned(length)))
    #define stdcall

#else
    //windows define
    #define inline_0 __declspec(noinline)
    #define inline_1 inline
    #define inline_2 __forceinline
    #define inline_3
    #define DLL_Export __declspec(dllexport)
    #define DLL_Import __declspec(dllimport)
    #define stdcall _stdcall
#endif

#if MODE_DEBUG
    #undef inline_1
    #undef inline_2
    #undef inline_3
    #define inline_1 inline
    #define inline_2 inline
    #define inline_3 inline
#endif

// msvc compiler define
#if defined(_MSC_VER)

// clang compiler define
#else
    #define HAS_CLANG_FEATURE(x) (__has_feature(x))
#endif

#define C_API_HEADER extern "C"
#define dll_export C_API_HEADER DLL_Export
#define ExportModule

#define Inline(i) inline_##i

#pragma region DEPRECATED

#if defined(_MSC_VER)
    #define DEPRECATED(msg) __declspec(deprecated(msg))
    #define DEPRECATED_ENUM_VALUE(msg) /* no equivalent for this in MSVC */
#elif defined(__clang__)
    #if __has_extension(attribute_deprecated_with_message)
        #define DEPRECATED(msg) __attribute__((deprecated(msg)))
    #else
        #define DEPRECATED(msg) __attribute__((deprecated))
    #endif

    #if __has_extension(enumerator_attributes)
        #if __has_extension(attribute_deprecated_with_message)
            #define DEPRECATED_ENUM_VALUE(msg) __attribute__((deprecated(msg)))
        #else
            #define DEPRECATED_ENUM_VALUE(msg) __attribute__((deprecated))
        #endif
    #else
        #define DEPRECATED_ENUM_VALUE(msg)
    #endif
#elif defined(__GNUC__)
    // Support for messages on the deprecated attribute arrived in GCC 4.5
    #if __GNUC__ >= 4 && __GNUC_MINOR__ >= 5
        #define DEPRECATED(msg) __attribute__((deprecated(msg)))
    #else
        #define DEPRECATED(msg) __attribute__((deprecated))
    #endif
    // Support for attributes on enumerators is GCC 6
    #if __GNUC__ >= 6
        #define DEPRECATED_ENUM_VALUE(msg) __attribute__((deprecated(msg)))
    #else
        #define DEPRECATED_ENUM_VALUE(msg)
    #endif
#else
    // TODO
    #define DEPRECATED(msg)
#endif

#pragma endregion

#pragma region ENUM_FLAGS
// Adds ability to use bit logic operators with enum type T.
// Enum must have appropriate values (e.g. kEnumValue1 = 1 << 1, kEnumValue2 = 1 << 2)!
#define ENUM_FLAGS(T) DETAIL__ENUM_FLAGS(T, )
#define ENUM_FLAGS_AS_MEMBER(T) DETAIL__ENUM_FLAGS(T, friend)

#define DETAIL__ENUM_FLAGS(T, PREFIX_)                                                                                 \
    static_assert(std::is_enum<T>::value);                                                                             \
    PREFIX_ inline T operator|(const T left, const T right) {                                                          \
        using type = std::underlying_type<T>::type;                                                                    \
        return static_cast<T>(static_cast<type>(left) | static_cast<type>(right));                                     \
    }                                                                                                                  \
    PREFIX_ inline T operator&(const T left, const T right) {                                                          \
        using type = std::underlying_type<T>::type;                                                                    \
        return static_cast<T>(static_cast<type>(left) & static_cast<type>(right));                                     \
    }                                                                                                                  \
    PREFIX_ inline T operator^(const T left, const T right) {                                                          \
        using type = std::underlying_type<T>::type;                                                                    \
        return static_cast<T>(static_cast<type>(left) ^ static_cast<type>(right));                                     \
    }                                                                                                                  \
    PREFIX_ inline T operator~(const T flags) {                                                                        \
        using type = std::underlying_type<T>::type;                                                                    \
        return static_cast<T>(~static_cast<type>(flags));                                                              \
    }                                                                                                                  \
    PREFIX_ inline T &operator|=(T &left, const T right) {                                                             \
        return left = left | right;                                                                                    \
    }                                                                                                                  \
    PREFIX_ inline T &operator&=(T &left, const T right) {                                                             \
        return left = left & right;                                                                                    \
    }                                                                                                                  \
    PREFIX_ inline T &operator^=(T &left, const T right) {                                                             \
        return left = left ^ right;                                                                                    \
    }                                                                                                                  \
    PREFIX_ inline bool HasFlag(const T flags, const T flagToTest) {                                                   \
        using type = std::underlying_type<T>::type;                                                                    \
        assert((static_cast<type>(flagToTest) & (static_cast<type>(flagToTest) - 1)) == 0 &&                           \
               "More than one flag specified in HasFlag()");                                                           \
        return (static_cast<type>(flags) & static_cast<type>(flagToTest)) != 0;                                        \
    }                                                                                                                  \
    PREFIX_ inline bool HasAnyFlags(const T flags, const T flagsToTest) {                                              \
        using type = std::underlying_type<T>::type;                                                                    \
        return (static_cast<type>(flags) & static_cast<type>(flagsToTest)) != 0;                                       \
    }                                                                                                                  \
    PREFIX_ inline bool HasAllFlags(const T flags, const T flagsToTest) {                                              \
        using type = std::underlying_type<T>::type;                                                                    \
        return (static_cast<type>(flags) & static_cast<type>(flagsToTest)) == static_cast<type>(flagsToTest);          \
    }                                                                                                                  \
    PREFIX_ inline T SetFlags(const T flags, const T flagsToSet) {                                                     \
        return (flags | flagsToSet);                                                                                   \
    }                                                                                                                  \
    PREFIX_ inline T ClearFlags(const T flags, const T flagsToClear) {                                                 \
        return (flags & ~flagsToClear);                                                                                \
    }                                                                                                                  \
    PREFIX_ inline T SetOrClearFlags(const T flags, const T flagsToSetOrClear, bool value) {                           \
        return value ? SetFlags(flags, flagsToSetOrClear) : ClearFlags(flags, flagsToSetOrClear);                      \
    }

// Adds ability to use increment operators with enum type T.
// Enum must have consecutive values!
#define ENUM_INCREMENT(T)                                                                                              \
    inline T &operator++(T &flags) {                                                                                   \
        flags = static_cast<T>(static_cast<int>(flags) + 1);                                                           \
        return flags;                                                                                                  \
    }                                                                                                                  \
    inline T operator++(T &flags, int) {                                                                               \
        T result = flags;                                                                                              \
        ++flags;                                                                                                       \
        return result;                                                                                                 \
    }
#pragma endregion

#define UNUSED_VAR(x) ((void)x)

#define NON_COPYABLE(Type)                                                                                             \
    Type(const Type &) = delete;                                                                                       \
    Type &operator=(const Type &) = delete
