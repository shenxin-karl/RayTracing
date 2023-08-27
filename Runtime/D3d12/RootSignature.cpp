#include "RootSignature.h"

#include <algorithm>

namespace dx {

#pragma region RootParameter

RootParameter::RootParameter() {
    ParameterType = static_cast<D3D12_ROOT_PARAMETER_TYPE>(0xFFFFFFFF);
}

RootParameter::~RootParameter() {
    Clear();
}

void RootParameter::Clear() {
    if (ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
        delete[] DescriptorTable.pDescriptorRanges;
        DescriptorTable.pDescriptorRanges = nullptr;
    }
    ParameterType = static_cast<D3D12_ROOT_PARAMETER_TYPE>(0xFFFFFFFF);
}

void RootParameter::InitAsBufferCBV(UINT Register, D3D12_SHADER_VISIBILITY visibility, UINT space) {
    ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    ShaderVisibility = visibility;
    Descriptor.ShaderRegister = Register;
    Descriptor.RegisterSpace = space;
}

void RootParameter::InitAsBufferSRV(UINT Register, D3D12_SHADER_VISIBILITY visibility, UINT space) {
    ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
    ShaderVisibility = visibility;
    Descriptor.ShaderRegister = Register;
    Descriptor.RegisterSpace = space;
}

void RootParameter::InitAsBufferUAV(UINT Register, D3D12_SHADER_VISIBILITY visibility, UINT space) {
    ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
    ShaderVisibility = visibility;
    Descriptor.ShaderRegister = Register;
    Descriptor.RegisterSpace = space;
}

void RootParameter::InitAsDescriptorTable(UINT rangeCount, D3D12_SHADER_VISIBILITY visibility) {
    ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    ShaderVisibility = visibility;
    DescriptorTable.NumDescriptorRanges = rangeCount;
    DescriptorTable.pDescriptorRanges = new D3D12_DESCRIPTOR_RANGE[rangeCount];
}

void RootParameter::InitAsDescriptorTable(std::initializer_list<D3D12_DESCRIPTOR_RANGE> ranges,
    D3D12_SHADER_VISIBILITY visibility) {

    ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    ShaderVisibility = visibility;
    DescriptorTable.NumDescriptorRanges = ranges.size();
    DescriptorTable.pDescriptorRanges = new D3D12_DESCRIPTOR_RANGE[ranges.size()];
    size_t index = 0;
    for (const D3D12_DESCRIPTOR_RANGE &range : ranges) {
        const D3D12_DESCRIPTOR_RANGE &dest = DescriptorTable.pDescriptorRanges[index++];
        const_cast<D3D12_DESCRIPTOR_RANGE &>(dest) = range;
    }
}

void RootParameter::SetTableRange(size_t rangeIndex,
    D3D12_DESCRIPTOR_RANGE_TYPE type,
    UINT Register,
    UINT count,
    UINT space) {

    D3D12_DESCRIPTOR_RANGE *pRange = const_cast<D3D12_DESCRIPTOR_RANGE *>(
        DescriptorTable.pDescriptorRanges + rangeIndex);
    pRange->RangeType = type;
    pRange->NumDescriptors = count;
    pRange->BaseShaderRegister = Register;
    pRange->RegisterSpace = space;
    pRange->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
}
#pragma endregion

#pragma region RootSignature

RootSignature::RootSignature() : _finalized(false), _numParameters(0), _numStaticSamplers(0), _numDescriptorPreTable{} {
}

RootSignature::~RootSignature() {
}

RootSignature::RootSignature(size_t numRootParam, size_t numStaticSamplers) : RootSignature() {
    Reset(numRootParam, numStaticSamplers);
}

void RootSignature::Reset(size_t numRootParam, size_t numStaticSamplers) {
    _finalized = false;
    _numParameters = numRootParam;
    _numStaticSamplers = numStaticSamplers;
    for (size_t i = 0; i < 2; ++i) {
        _descriptorTableBitMap[i].reset();
        std::ranges::fill(_numDescriptorPreTable[i], 0);
    }
    _pRootSignature = nullptr;
    _rootParameters.resize(numRootParam);
    _staticSamplers.resize(numStaticSamplers);
}

void RootSignature::InitStaticSamplers(const std::vector<D3D12_STATIC_SAMPLER_DESC> &descs, size_t offset) {
    Assert(offset < _numStaticSamplers);
    size_t i = 0;
    while (i < descs.size()) {
	    if (i + offset < _numStaticSamplers) {
		    _staticSamplers[i+offset] = descs[i];
	    }
        ++i;
    }
}

void RootSignature::SetStaticSampler(size_t index, const D3D12_STATIC_SAMPLER_DESC &desc) {
    Assert(index < _numStaticSamplers);
    _staticSamplers[index] = desc;
}

auto RootSignature::GetRootSignature() const -> ID3D12RootSignature * {
    return _pRootSignature.Get();
}

auto RootSignature::At(size_t index) -> RootParameter & {
    Assert(index < _numParameters);
    return _rootParameters[index];
}

bool RootSignature::IsFinalized() const {
    return _finalized;
}

void RootSignature::Finalize(D3D12_ROOT_SIGNATURE_FLAGS flags) {
}
#pragma endregion

}    // namespace dx
