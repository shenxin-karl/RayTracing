#pragma once
#include <bitset>
#include <array>
#include "D3dUtils.h"
#include "Foundation/ReadonlyArraySpan.hpp"

namespace dx {

// clang-format off
class RootParameter : public D3D12_ROOT_PARAMETER {
public:
    RootParameter();
    ~RootParameter();
    void Clear(); 
	void InitAsConstants(UINT num32BitValues, UINT shaderRegister, UINT registerSpace = 0, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
    void InitAsBufferCBV(UINT Register, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL, UINT space = 0);
    void InitAsBufferSRV(UINT Register, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL, UINT space = 0);
    void InitAsBufferUAV(UINT Register, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL, UINT space = 0);
    void InitAsDescriptorTable(UINT rangeCount, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
    void InitAsDescriptorTable(std::initializer_list<D3D12_DESCRIPTOR_RANGE> ranges, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
    void SetTableRange(size_t rangeIndex, D3D12_DESCRIPTOR_RANGE_TYPE type, UINT Register, UINT count, UINT space = 0);
};
// clang-format on

class RootSignature : NonCopyable {
private:
    using DescriptorTableBitMask = std::bitset<kMaxRootParameter>;
    using NumDescriptorPreTable = std::array<uint8_t, kMaxRootParameter>;
public:
    RootSignature();
    ~RootSignature();
    RootSignature(size_t numRootParam, size_t numStaticSamplers = 0);
public:
    void Reset(size_t numRootParam, size_t numStaticSamplers = 0);
    void InitStaticSamplers(ReadonlyArraySpan<D3D12_STATIC_SAMPLER_DESC> descs, size_t offset = 0);
    void SetStaticSampler(size_t index, const D3D12_STATIC_SAMPLER_DESC &desc);
    auto GetRootSignature() const -> ID3D12RootSignature *;
    auto At(size_t index) -> RootParameter &;
    bool IsFinalized() const;
    void Finalize(Device *pDevice,
        D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    auto GetDescriptorTableBitMask(D3D12_DESCRIPTOR_HEAP_TYPE heapType) const -> DescriptorTableBitMask;
    auto GetNumDescriptorPreTable(D3D12_DESCRIPTOR_HEAP_TYPE heapType) const -> const NumDescriptorPreTable &;
private:
    // clang-format off
    bool                                    _finalized;
    size_t                                  _numParameters;
    size_t                                  _numStaticSamplers;
    DescriptorTableBitMask                  _descriptorTableBitMap[2];
    NumDescriptorPreTable                   _numDescriptorPreTable[2];
    WRL::ComPtr<ID3D12RootSignature>        _pRootSignature;
    std::vector<RootParameter>              _rootParameters;
    std::vector<D3D12_STATIC_SAMPLER_DESC>  _staticSamplers;
    // clang-format on
};

}    // namespace dx
