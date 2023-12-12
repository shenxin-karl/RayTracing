#pragma once
#include "PreprocessorDirectives.h"
#include <source_location>
#include <string>
#include <fmt/format.h>

struct FormatAndLocation {
    std::string_view fmt;
    std::source_location location;
public:
    template<size_t N>
    FormatAndLocation(const char (&arr)[N], const std::source_location &l = std::source_location::current())
        : fmt(arr), location(l) {
    }
    FormatAndLocation(const std::string_view &fmt, const std::source_location &l = std::source_location::current())
        : fmt(fmt), location(l) {
    }
    FormatAndLocation(const std::string &fmt, const std::source_location &l = std::source_location::current())
        : fmt(fmt.c_str(), fmt.length()), location(l) {
    }
    FormatAndLocation(const char *cString, const std::source_location &l = std::source_location::current())
        : fmt(cString, (cString != nullptr ? strlen(cString) : 0)), location(l) {
    }
};

class Exception : public std::exception {
public:
    Exception(const std::string &message, const std::source_location &sourceLocation);
    auto what() const noexcept -> const char * override;
    auto GetLine() const noexcept -> int;
    auto GetFile() const noexcept -> const char *;
    auto GetFunc() const noexcept -> const char *;
    auto GetErrorMessage() const noexcept -> const std::string &;

    template<typename... Args>
    static void Throw(const FormatAndLocation &fmtAndLocation, Args &&...args) {
        std::string message;
        if constexpr (sizeof...(Args)) {
            message = fmt::vformat(fmtAndLocation.fmt, fmt::make_format_args(args...));
        } else {
            message = fmtAndLocation.fmt.data();
        }
        ThrowException(Exception(std::move(message), fmtAndLocation.location));
    }
    template<typename... Args>
    static void CondThrow(bool cond, const FormatAndLocation &fmtAndLocation, Args &&...args) {
        if (!cond) {
            Exception::Throw(fmtAndLocation, std::forward<Args>(args)...);
        }
    }
protected:
    static void ThrowException(const Exception &exception) noexcept(false);
protected:
    int _line;
    const char *_file;
    const char *_func;
    std::string _message;
    std::string _whatBuffer;
};

#define Assert(cond)                                                                                                   \
    do {                                                                                                               \
        bool _bValueFlag = bool(cond);                                                                                 \
        ::Exception::CondThrow(_bValueFlag,                                                                            \
            fmt::format("In Function {}, Assert({}) Failed!", __FUNCTION_NAME__, #cond));                              \
    } while (false)

class NotImplementedException : public std::exception {
public:
    explicit NotImplementedException(const char *func);
    auto what() const noexcept -> const char * override;
    static void Throw(const std::source_location &sourceLocation = std::source_location::current()) {
        throw NotImplementedException(sourceLocation.function_name());
    }
private:
    const char *_func;
    mutable std::string _whatBuffer;
};
