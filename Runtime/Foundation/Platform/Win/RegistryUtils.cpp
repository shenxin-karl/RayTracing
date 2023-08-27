#include "RegistryUtils.h"
#ifdef PLATFORM_WIN
    #include "Foundation/StringConvert.h"

namespace Registry {

std::optional<std::string> GetString(HKEY hkey, std::wstring_view path, std::wstring_view key) {
    LONG result = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, path.data(), 0, KEY_READ, &hkey);
    if (result != ERROR_SUCCESS) {
        return std::nullopt;
    }

    DWORD dataType;
    WCHAR buffer[2048];
    DWORD dataLength = sizeof(buffer);
    result = RegQueryValueEx(hkey, key.data(), nullptr, &dataType, reinterpret_cast<LPBYTE>(&buffer), &dataLength);
    ::RegCloseKey(hkey);
    if (result != ERROR_SUCCESS) {
        return std::nullopt;
    }

    if ((dataType == REG_SZ || dataType == REG_MULTI_SZ) && dataLength >= sizeof(WCHAR) && result == ERROR_SUCCESS) {
        const DWORD outLength = dataLength / sizeof(WCHAR) - 1;
        return std::string(buffer, buffer + outLength);
    }
    if ((dataType == REG_EXPAND_SZ) && dataLength >= 1 && result == ERROR_SUCCESS) {
        // expand the string
        WCHAR exBuffer[ARRAYSIZE(buffer)];
        if (!::ExpandEnvironmentStrings(buffer, exBuffer, ARRAYSIZE(exBuffer) - 1)) {
            const DWORD outLength = dataLength / sizeof(WCHAR) - 1;
            return std::string(buffer, buffer + outLength);
        } else {
            std::wstring value = exBuffer;
            return nstd::to_string(value);
        }
    }
    return std::nullopt;
}

}    // namespace Registry

#endif