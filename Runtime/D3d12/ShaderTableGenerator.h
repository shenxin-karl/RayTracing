#pragma once
#include "D3dUtils.h"
#include "Foundation/MemoryStream.h"

namespace dx {

class ShaderRecode {
public:
    ShaderRecode(void *pShaderIdentifier) : _pShaderIdentifier(pShaderIdentifier) {
    }
    template<typename... Args>
    ShaderRecode(void *pShaderIdentify, Args &&...args) : _pShaderIdentifier(nullptr) {
        if constexpr (sizeof...(Args) > 0) {
            _memoryStream.Reverse(GetSize<Args...>());
            AppendObject(std::forward<Args>(args)...);
        }
    }
    template<typename T>
    auto AddParameter(T &&obj) -> ShaderRecode & {
	    _memoryStream.Append(obj);
        return *this;
    }
    auto GetStride() const -> size_t {
        return AlignUp<size_t>(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + _memoryStream.GetSize(),
            D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    }
private:
    template<typename T, typename... Args>
    constexpr static size_t GetSize() {
        if constexpr (sizeof...(Args) > 0) {
            return sizeof(T) + GetSize<Args...>();
        }
        return sizeof(T);
    }
    template<typename T, typename... Args>
    void AppendObject(T &&arg, Args &&...args) {
        _memoryStream.Append(std::forward<T>(args));
        if constexpr (sizeof...(Args) > 0) {
            AppendObject(std::forward<Args>(args)...);
        }
    }
private:
    // clang-format off
	void					*_pShaderIdentifier = nullptr;
    MemoryStream             _memoryStream    = {};
    // clang-format on
};

class ShaderTableGenerator : NonCopyable {
public:
    ShaderTableGenerator();
    //void AddShaderRecode(ShaderRecode )
    auto Generate(Context *pContext) -> D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE;

    void Add(ShaderRecode &&shaderRecode) {
	    _shaderRecodes.push_back(std::move(shaderRecode));
    }
    template<typename ...Args>
    void Emplace(Args&&...args) -> ShaderRecode & {
	    _shaderRecodes.emplace_back(std::forward<Args>(args)...);
        return _shaderRecodes.back();
    }
private:
    // clang-format off
	std::vector<ShaderRecode> _shaderRecodes;
    // clang-format on
};

}    // namespace dx
