#include "StringConvert.h"
#include <locale>
#include <codecvt>

namespace nstd {

static std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

std::string to_string(const std::wstring &string) {
    return converter.to_bytes(string);
}

std::string to_string(std::wstring_view string) {
    return converter.to_bytes(string.data(), string.data() + string.length());
}

std::string to_string(const wchar_t *pString) {
    if (pString == nullptr) {
        return std::string{};
    }
    return converter.to_bytes(pString, pString + std::strlen(reinterpret_cast<const char *>(pString)));
}

std::wstring to_wstring(const std::string &string) {
    return converter.from_bytes(string);
}

std::wstring to_wstring(std::string_view string) {
    return converter.from_bytes(string.data(), string.data() + string.length());
}

std::wstring to_wstring(const char *pString) {
    if (pString == nullptr) {
        return std::wstring{};
    }
    return converter.from_bytes(pString, pString + std::strlen(pString));
}

}    // namespace nstd
