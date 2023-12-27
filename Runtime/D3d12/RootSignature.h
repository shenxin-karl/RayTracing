#pragma once
#include <bitset>
#include <array>
#include "D3dUtils.h"
#include "Foundation/ReadonlyArraySpan.hpp"

namespace dx {

// clang-format off
class RootParameter : public D3D12_ROOT_PARAMETER1 {
public:
    RootParameter();
    ~RootParameter();
    void Clear(); 
	void InitAsConstants(UINT num32BitValues, UINT shaderRegister, UINT registerSpace = 0, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
    void InitAsBufferCBV(UINT Register, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL, UINT space = 0);
    void InitAsBufferSRV(UINT Register, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL, UINT space = 0);
    void InitAsBufferUAV(UINT Register, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL, UINT space = 0);
    void InitAsDescriptorTable(UINT rangeCount, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
    void InitAsDescriptorTable(std::initializer_list<D3D12_DESCRIPTOR_RANGE1> ranges, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
    void SetTableRange(size_t rangeIndex, D3D12_DESCRIPTOR_RANGE_TYPE type, UINT Register, UINT count, UINT space = 0, D3D12_DESCRIPTOR_RANGE_FLAGS Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE);
};
// clang-format on

class RootSignature : NonCopyable {
public:
    using DescriptorTableBitMask = std::bitset<kMaxRootParameter>;
    using NumDescriptorPreTable = std::array<uint8_t, kMaxRootParameter>;
    struct DescriptorTableInfo {
        uint8_t numDescriptor;
        bool enableBindless = false;
    };
    using RootParamDescriptorTableInfo = std::array<DescriptorTableInfo, kMaxRootParameter>;
public:
    RootSignature();
    ~RootSignature();
public:
    void OnCreate(size_t numRootParam, size_t numStaticSamplers = 0);
    void OnDestroy();
    void SetStaticSamplers(ReadonlyArraySpan<D3D12_STATIC_SAMPLER_DESC> descs, size_t offset = 0);
    void SetStaticSampler(size_t index, const D3D12_STATIC_SAMPLER_DESC &desc);
    auto GetRootSignature() const -> ID3D12RootSignature *;
    auto At(size_t index) -> RootParameter &;
    void Generate(Device *pDevice, D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    auto GetRootParamDescriptorTableInfo(D3D12_DESCRIPTOR_HEAP_TYPE heapType) const -> const  RootParamDescriptorTableInfo &;
    auto GetDescriptorTableBitMask(D3D12_DESCRIPTOR_HEAP_TYPE heapType) const -> DescriptorTableBitMask;
    void SetName(std::string_view name);
private:
    // clang-format off
    std::string                             _name;
    size_t                                  _numParameters;
    size_t                                  _numStaticSamplers;
	RootParamDescriptorTableInfo            _descriptorTableInfo[2];
    DescriptorTableBitMask                  _descriptorTableBitMap[2];
    WRL::ComPtr<ID3D12RootSignature>        _pRootSignature;
    std::vector<RootParameter>              _rootParameters;
    std::vector<D3D12_STATIC_SAMPLER_DESC>  _staticSamplers;
    // clang-format on
};

}    // namespace dx
