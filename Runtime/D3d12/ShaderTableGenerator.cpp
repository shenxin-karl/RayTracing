#include "ShaderTableGenerator.h"
#include "Context.h"

namespace dx {

void ShaderRecode::CopyToBuffer(void *pDest, size_t destBufferSize) const {
    uint8_t *ptr = static_cast<uint8_t *>(pDest);
    std::memcpy(ptr, _pShaderIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    ptr += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

    if (_memoryStream.GetSize() > 0) {
        std::memcpy(ptr, _memoryStream.GetData(), _memoryStream.GetSize());
        ptr += _memoryStream.GetSize();
    }

    size_t remainingBufferSize = destBufferSize - GetStride();
    if (remainingBufferSize > 0) {
        std::memset(ptr, 0, remainingBufferSize);
    }
}

ShaderTableGenerator::ShaderTableGenerator() {
}

constexpr size_t kAddressAlignment = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES * 2;
auto ShaderTableGenerator::Generate(Context *pContext) -> D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE {
    Assert(!_shaderRecodes.empty());
    size_t stride = 0;
    for (ShaderRecode &shaderRecode : _shaderRecodes) {
        stride = std::max(stride, shaderRecode.GetStride());
    }

    size_t bufferSize = stride * _shaderRecodes.size();
    DynamicBufferAllocator::AllocInfo allocInfo = pContext->AllocBuffer(bufferSize, kAddressAlignment);

    uint8_t *pDest = allocInfo.pBuffer;
    for (ShaderRecode &shaderRecode : _shaderRecodes) {
        shaderRecode.CopyToBuffer(pDest, stride);
        pDest += stride;
    }

    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE result = {};
    result.StartAddress = allocInfo.virtualAddress;
    result.SizeInBytes = bufferSize;
    result.StrideInBytes = stride;
    return result;
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE MakeRayGenShaderRecode(Context *pContext, const ShaderRecode &shaderRecode) {
    size_t bufferSize = shaderRecode.GetStride();
    DynamicBufferAllocator::AllocInfo allocInfo = pContext->AllocBuffer(bufferSize, kAddressAlignment);
    shaderRecode.CopyToBuffer(allocInfo.pBuffer, bufferSize);

    D3D12_GPU_VIRTUAL_ADDRESS_RANGE result = {};
    result.StartAddress = allocInfo.virtualAddress;
    result.SizeInBytes = bufferSize;
    return result;
}

}    // namespace dx
