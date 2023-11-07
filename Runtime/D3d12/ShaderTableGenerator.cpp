#include "ShaderTableGenerator.h"
#include "Context.h"

namespace dx {

ShaderTableGenerator::ShaderTableGenerator() {
}

auto ShaderTableGenerator::Generate(Context *pContext) -> D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE {
    constexpr size_t kAddressAlignment = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES * 2;
    Assert(!_shaderRecodes.empty());
    size_t stride = 0;
    for (ShaderRecode &shaderRecode : _shaderRecodes) {
	    stride = std::max(stride, shaderRecode.GetStride());
    }

    size_t shaderIdentifierSize = GetSizeInBytes();
    DynamicBufferAllocator::AllocInfo allocInfo = pContext->AllocBuffer(shaderIdentifierSize, kAddressAlignment);

    uint8_t *pDest = allocInfo.pBuffer;
    for (const void *pShaderIdentifier : _identifierList) {
        std::memcpy(pDest, pShaderIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
        pDest += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    }
    _startAddress = allocInfo.virtualAddress;

    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE result = {};
    result.StartAddress = GetStartAddress();
    result.SizeInBytes = GetSizeInBytes();
    result.StrideInBytes = GetStrideInBytes();
    return result;
}

}    // namespace dx
