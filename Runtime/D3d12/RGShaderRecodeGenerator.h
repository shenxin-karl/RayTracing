#pragma once
#include "D3dUtils.h"

namespace dx {

class RGShaderRecodeGenerator : public NonCopyable {
public:
    template<typename T>
    RGShaderRecodeGenerator(const void *pShaderIdentifier, const T &constantBuffer)
        : RGShaderRecodeGenerator(pShaderIdentifier, &constantBuffer, sizeof(T)) {
    }
    RGShaderRecodeGenerator(const void *pShaderIdentifier) : RGShaderRecodeGenerator(pShaderIdentifier, nullptr, 0) {
	    
    }
    RGShaderRecodeGenerator(const void *pShaderIdentifier, const void *pConstantBuffer, size_t bufferSize);
    auto Generate(Context *pContext) -> D3D12_GPU_VIRTUAL_ADDRESS_RANGE;
    auto GetStartAddress() const -> D3D12_GPU_VIRTUAL_ADDRESS {
        return _startAddress;
    }
    auto GetSizeInBytes() const -> std::size_t {
        return _buffer.size();
    }
private:
    // clang-format off
	std::vector<std::byte>		_buffer;
	D3D12_GPU_VIRTUAL_ADDRESS	_startAddress;
    // clang-format on
};

}    // namespace dx
