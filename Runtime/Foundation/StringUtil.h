#pragma once
#include <string>
#include <vector>

namespace nstd {

std::string to_string(const std::wstring &string);
std::string to_string(std::wstring_view string);
std::string to_string(const wchar_t *pString);

std::wstring to_wstring(const std::string &string);
std::wstring to_wstring(std::string_view string);
std::wstring to_wstring(const char *pString);

auto Base64Encode(const uint8_t *pData, size_t len) -> std::string;
auto Base64Decode(std::string_view encodedData) -> std::vector<uint8_t>;

}
