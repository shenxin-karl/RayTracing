#include "StringUtil.h"
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

static constexpr std::string_view kBase64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
auto Base64Encode(const uint8_t *pData, size_t len) -> std::string {
    std::string encoded;

    int i = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (len--) {
        char_array_3[i++] = *(pData++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; (i < 4); i++)
                encoded += kBase64Chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        int j = 0;
        for (j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

        for (j = 0; (j < i + 1); j++)
            encoded += kBase64Chars[char_array_4[j]];

        while ((i++ < 3))
            encoded += '=';
    }

    return encoded;
}

auto Base64Decode(std::string_view encodedData) -> std::vector<uint8_t> {
    std::vector<unsigned char> decoded_data;
    int i = 0;
    int j = 0;
    unsigned char char_array_4[4], char_array_3[3];

    for (const auto &c : encodedData) {
        if (c == '=') {
            break;
        }

        if (kBase64Chars.find(c) == std::string::npos) {
            continue;
        }

        char_array_4[i++] = c;
        if (i == 4) {
            for (i = 0; i < 4; i++) {
                char_array_4[i] = kBase64Chars.find(char_array_4[i]);
            }

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; i < 3; i++) {
                decoded_data.push_back(char_array_3[i]);
            }

            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 4; j++) {
            char_array_4[j] = 0;
        }

        for (j = 0; j < 4; j++) {
            char_array_4[j] = kBase64Chars.find(char_array_4[j]);
        }

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);

        for (j = 0; j < i - 1; j++) {
            decoded_data.push_back(char_array_3[j]);
        }
    }
    return decoded_data;
}

}    // namespace nstd
