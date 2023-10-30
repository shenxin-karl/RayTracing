#pragma once
#include "D3dUtils.h"

namespace dx {

class ShaderTableGenerator : NonCopyable {
public:
	ShaderTableGenerator();
	auto Generate(Context *pContext) -> D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE;

	void AddShaderIdentifier(const void *pShaderIdentifier) {
		_identifierList.push_back(pShaderIdentifier);
	}
	auto GetStartAddress() const -> D3D12_GPU_VIRTUAL_ADDRESS {
		return 	_startAddress;
	}
	auto GetSizeInBytes() const -> size_t {
		return _identifierList.size() * D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	}
	static constexpr auto GetStrideInBytes() noexcept -> size_t {
		return D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	}
private:
	// clang-format off
	std::vector<const void *>	_identifierList;
	D3D12_GPU_VIRTUAL_ADDRESS	_startAddress;
	// clang-format on
};

}
