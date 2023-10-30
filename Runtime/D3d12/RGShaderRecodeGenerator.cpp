#include "RGShaderRecodeGenerator.h"
#include "Context.h"

namespace dx {

RGShaderRecodeGenerator::RGShaderRecodeGenerator(const void *pShaderIdentifier,
    const void *pConstantBuffer,
    size_t bufferSize)
    : _startAddress(0) {

    _buffer.resize(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + bufferSize);
    std::memcpy(_buffer.data(), pShaderIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    std::memcpy(_buffer.data() + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, pConstantBuffer, bufferSize);
}

auto RGShaderRecodeGenerator::Generate(Context *pContext) -> D3D12_GPU_VIRTUAL_ADDRESS_RANGE {
    constexpr size_t kAddressAlignment = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES * 2;
    DynamicBufferAllocator::AllocInfo rayGenAllocInfo = pContext->AllocBuffer(_buffer.size(), kAddressAlignment);
    std::memcpy(rayGenAllocInfo.pBuffer, _buffer.data(), _buffer.size());
    _startAddress = rayGenAllocInfo.virtualAddress;

    D3D12_GPU_VIRTUAL_ADDRESS_RANGE result = {};
    result.StartAddress = _startAddress;
    result.SizeInBytes = _buffer.size();
    return result;
}

}    // namespace dx
