#pragma once
#include <spdlog/sinks/basic_file_sink.h>
#include "Singleton.hpp"
#include "RuntimeStatic.h"
#include "NamespeceAlias.h"
#include "Exception.h"
#include "PreprocessorDirectives.h"

class Logger : public Singleton<Logger> {
public:
    enum DebugLevel {
        // clang-format off
        eNone       = 0,
        eInfo       = 1 << 0,
        eDebug      = 1 << 1,
        eWarning    = 1 << 2,
        eError      = 1 << 3,
        eAll        = eInfo | eDebug | eWarning | eError,
        // clang-format on
    };
    ENUM_FLAGS_AS_MEMBER(DebugLevel)
    using LogCallBack = std::function<void(DebugLevel, const std::string &)>;
private:
    void LogMessage(DebugLevel level, const std::source_location &location, const std::string &message);
public:
    void OnCreate();
    void OnDestroy();
    void SetLogPath(stdfs::path logPath);
    void SetLogLevelMask(DebugLevel mask);
    void SetPattern(const std::string &pattern);
    void SetLogCallBack(const LogCallBack &callback);
    auto GetLogLevelMask() const -> DebugLevel;

    template<typename... Args>
    static void Info(FormatAndLocation fmtAndLoc, Args &&...args);

    template<typename... Args>
    static void Debug(FormatAndLocation fmtAndLoc, Args &&...args);

    template<typename... Args>
    static void Warning(FormatAndLocation fmtAndLoc, Args &&...args);

    template<typename... Args>
    static void Error(FormatAndLocation fmtAndLoc, Args &&...args);
private:
    DebugLevel _logLevelMask = eAll;
    std::string _pattern;
    stdfs::path _logFilePath;
    LogCallBack _logCallback;
    std::shared_ptr<spdlog::logger> _pLogger;
    std::shared_ptr<spdlog::logger> _pConsoleLogger;
};

template<typename... Args>
void Logger::Info(FormatAndLocation fmtAndLoc, Args &&...args) {
    if (GetInstance() == nullptr || !HasFlag(GetInstance()->GetLogLevelMask(), eInfo)) {
        return;
    }
    std::string message = fmt::vformat(fmtAndLoc.fmt, fmt::make_format_args(std::forward<Args>(args)...));
    GetInstance()->LogMessage(eInfo, fmtAndLoc.location, message);
}

template<typename... Args>
void Logger::Debug(FormatAndLocation fmtAndLoc, Args &&...args) {
    if (GetInstance() == nullptr || !HasFlag(GetInstance()->GetLogLevelMask(), eDebug)) {
        return;
    }
    std::string message = fmt::vformat(fmtAndLoc.fmt, fmt::make_format_args(std::forward<Args>(args)...));
    GetInstance()->LogMessage(eDebug, fmtAndLoc.location, message);
}

template<typename... Args>
void Logger::Warning(FormatAndLocation fmtAndLoc, Args &&...args) {
    if (GetInstance() == nullptr || !HasFlag(GetInstance()->GetLogLevelMask(), eWarning)) {
        return;
    }
    std::string message = fmt::vformat(fmtAndLoc.fmt, fmt::make_format_args(std::forward<Args>(args)...));
    GetInstance()->LogMessage(eWarning, fmtAndLoc.location, message);
}

template<typename... Args>
void Logger::Error(FormatAndLocation fmtAndLoc, Args &&...args) {
    if (GetInstance() == nullptr || !HasFlag(GetInstance()->GetLogLevelMask(), eError)) {
        return;
    }
    std::string message = fmt::vformat(fmtAndLoc.fmt, fmt::make_format_args(std::forward<Args>(args)...));
    GetInstance()->LogMessage(eError, fmtAndLoc.location, message);
}
