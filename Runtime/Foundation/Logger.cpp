#include "Logger.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

void Logger::LogMessage(DebugLevel level, const std::source_location &location, const std::string &message) {
    std::string stringToWrite;
    const char *pFileName = location.file_name();
    const uint32_t line = location.line();
    const uint32_t column = location.column();
    std::string output;
    switch (level) {
    case eInfo:
        output = fmt::format("{}({},{}) [info ]: {}", pFileName, line, column, message);
        _pLogger->info("{}", output);
        _pConsoleLogger->info("{}", output);
        break;
    case eDebug:
        output = fmt::format("{}({},{}) [Debug]: {}", pFileName, line, column, message);
        _pLogger->debug("{}", output);
        _pConsoleLogger->debug("{}", output);
        break;
    case eWarning:
        output = fmt::format("{}({},{}) [Warn ]: {}", pFileName, line, column, message);
        _pLogger->warn("{}", output);
        _pConsoleLogger->warn("{}", output);
        break;
    case eError:
        output = fmt::format("{}({},{}) [Error]: {}", pFileName, line, column, message);
        _pLogger->error("{}", output);
        _pConsoleLogger->error("{}", output);
        break;
    case eNone:
    default:
        return;
    }

    if (_logCallback != nullptr) {
        _logCallback(level, output);
    }

#if PLATFORM_WIN
    OutputDebugStringA(output.c_str());
#endif
}

void Logger::OnCreate() {
    _logLevelMask = eInfo | eDebug | eWarning | eError;
    _pattern = "[%H:%M:%S] %v";
    _logFilePath = "log.txt";

    _pLogger = spdlog::basic_logger_mt("basic_logger", "logs/basic-log.txt");
    _pConsoleLogger = spdlog::stdout_color_mt("console");
    spdlog::set_pattern(_pattern.c_str());
}

void Logger::OnDestroy() {
    _pLogger = nullptr;
}

void Logger::SetLogPath(stdfs::path logPath) {
    _logFilePath = logPath;
}

void Logger::SetLogLevelMask(DebugLevel mask) {
    _logLevelMask = mask;
}

void Logger::SetPattern(const std::string &pattern) {
    _pattern = pattern;
}

void Logger::SetLogCallBack(const LogCallBack &callback) {
    _logCallback = callback;
}

auto Logger::GetLogLevelMask() const -> DebugLevel {
    return _logLevelMask;
}
