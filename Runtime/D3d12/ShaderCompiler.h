#pragma once
#include <optional>
#include <string>
#include <vector>
#include <functional>
#include "D3dUtils.h"
#include "Foundation/CompileEnvInfo.hpp"
#include "Foundation/NonCopyable.h"
#include "Foundation/NamespeceAlias.h"

namespace dx {

enum class ShaderType {
    eVS = 1,
    eHS = 2,
    eDS = 3,
    eGS = 4,
    ePS = 5,
    eCS = 6,
};

#pragma region DefineList
class DefineList : public NonCopyable {
public:
    struct MacroItem {
        std::string key;
        int value;
    };
    using iterator = std::vector<MacroItem>::iterator;
    using const_iterator = std::vector<MacroItem>::const_iterator;
public:
    DefineList() = default;
    void Set(std::string_view key, int value = 1);
    auto Get(std::string_view key) const -> std::optional<int>;
    bool Remove(std::string_view key);
    auto operator[](std::string_view key) -> int &;
    auto ToString() const -> std::string;
    auto FromString(std::string source) -> size_t;

    void Clear() {
        _macroItems.clear();
    }
    auto GetCount() const -> size_t {
        return _macroItems.size();
    }
    auto begin() const -> const_iterator {
        return _macroItems.begin();
    }
    auto end() const -> const_iterator {
        return _macroItems.end();
    }
private:
    auto Find(std::string_view key) const -> iterator;
private:
    mutable std::vector<MacroItem> _macroItems;
};

#pragma endregion

#pragma region ShaderCompiler

/**
 * \brief 
 * \param path          [in]    hlsl include file path
 * \param fileContent   [out]   return file content buffer
 * \return                      return success status
 */
using ShaderIncludeCallback = std::function<bool(const std::string &path, std::string &fileContent)>;

struct ShaderCompilerDesc {
    stdfs::path path;
    std::string_view entryPoint;
    ShaderType shaderType;
    const DefineList *pDefineList = nullptr;
    bool makeDebugInfo = !CompileEnvInfo::IsModeRelWithDebInfo();
    ShaderIncludeCallback includeCallback;
};

class ShaderCompiler : NonCopyable {
public:
    bool Compile(const ShaderCompilerDesc &desc);
    auto GetErrorMessage() const -> const std::string &;
    auto GetByteCode() const -> WRL::ComPtr<IDxcBlob>;
private:
    // clang-format off
    HRESULT                 _result = 0;
    std::string             _errorMessage;
    WRL::ComPtr<IDxcBlob>   _pByteCode;
    // clang-format on
};
#pragma endregion

}    // namespace dx
