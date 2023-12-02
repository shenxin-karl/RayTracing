#pragma once
#include <fmt/format.h>
#include <filesystem>

template<>
struct fmt::formatter<std::string_view> {
    template<typename ParseContext>
    static constexpr auto parse(ParseContext &ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const std::string_view &string, FormatContext &ctx) {
        return fmt::format_to(ctx.out(), "{}", string.data());
    }
};

template<>
struct fmt::formatter<std::wstring_view> {
    template<typename ParseContext>
    static constexpr auto parse(ParseContext &ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const std::wstring_view &string, FormatContext &ctx) {
        return fmt::format_to(ctx.out(), "{}", string.data());
    }
};

template<>
struct fmt::formatter<std::string> {
    template<typename ParseContext>
    static constexpr auto parse(ParseContext &ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const std::string &string, FormatContext &ctx) {
        return fmt::format_to(ctx.out(), "{}", string.c_str());
    }
};

template<>
struct fmt::formatter<std::wstring> {
    template<typename ParseContext>
    static constexpr auto parse(ParseContext &ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const std::wstring &string, FormatContext &ctx) {
        return fmt::format_to(ctx.out(), "{}", string.c_str());
    }
};

template<>
struct fmt::formatter<std::filesystem::path> {
    template<typename ParseContext>
    static constexpr auto parse(ParseContext &ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const std::filesystem::path &path, FormatContext &ctx) {
        return fmt::format_to(ctx.out(), "{}", path.string().c_str());
    }
};


