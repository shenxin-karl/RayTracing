#pragma once
#include <bitset>
#include <array>
#include "D3dUtils.h"

namespace dx {

// clang-format off
class RootParameter : public D3D12_ROOT_PARAMETER {
public:
    RootParameter();
    ~RootParameter();
    void Clear();
    void InitAsBufferCBV(UINT Register, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL, UINT space = 0);
    void InitAsBufferSRV(UINT Register, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL, UINT space = 0);
    void InitAsBufferUAV(UINT Register, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL, UINT space = 0);
    void InitAsDescriptorTable(UINT rangeCount, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
    void InitAsDescriptorTable(std::initializer_list<D3D12_DESCRIPTOR_RANGE> ranges, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
    void SetTableRange(size_t rangeIndex, D3D12_DESCRIPTOR_RANGE_TYPE type, UINT Register, UINT count, UINT space = 0);
};
// clang-format on

class RootSignature : NonCopyable {
public:
    RootSignature();
    ~RootSignature();
    RootSignature(size_t numRootParam, size_t numStaticSamplers = 0);
public:
    void Reset(size_t numRootParam, size_t numStaticSamplers = 0);
    void InitStaticSamplers(const std::vector<D3D12_STATIC_SAMPLER_DESC> &descs, size_t offset = 0);
    void SetStaticSampler(size_t index, const D3D12_STATIC_SAMPLER_DESC &desc);
    auto GetRootSignature() const -> ID3D12RootSignature *;
    auto At(size_t index) -> RootParameter &;
    bool IsFinalized() const;
    void Finalize(D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
private:
    bool _finalized;
    size_t _numParameters;
    size_t _numStaticSamplers;
    std::bitset<16> _descriptorTableBitMap[2];
    std::array<uint8_t, 16> _numDescriptorPreTable[2];
    WRL::ComPtr<ID3D12RootSignature> _pRootSignature;
    std::vector<RootParameter> _rootParameters;
    std::vector<D3D12_STATIC_SAMPLER_DESC> _staticSamplers;
};

}    // namespace dx