#pragma once
#include <string>

namespace nstd {

std::string to_string(const std::wstring &string);
std::string to_string(std::wstring_view string);
std::string to_string(const wchar_t *pString);

std::wstring to_wstring(const std::string &string);
std::wstring to_wstring(std::string_view string);
std::wstring to_wstring(const char *pString);
	

}
