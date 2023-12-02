#include "RootSignature.h"
#include "Device.h"
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

void RootParameter::InitAsConstants(UINT num32BitValues,
    UINT shaderRegister,
    UINT registerSpace,
    D3D12_SHADER_VISIBILITY visibility) {

    ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    ShaderVisibility = visibility;
    CD3DX12_ROOT_CONSTANTS::Init(Constants, num32BitValues, shaderRegister, registerSpace);
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

constexpr static size_t kCBV_UAV_SRV_HeapIndex = 0;
constexpr static size_t kSAMPLER_HeapIndex = 1;

static size_t GetPerTableIndexByRangeType(D3D12_DESCRIPTOR_RANGE_TYPE type) {
    switch (type) {
    case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
    case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
    case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
        return kCBV_UAV_SRV_HeapIndex;
    case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
        return kSAMPLER_HeapIndex;
    }
    Exception::Throw("invalid D3D12_DESCRIPTOR_RANGE_TYP");
    return -1;
}

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

void RootSignature::InitStaticSamplers(ReadonlyArraySpan<D3D12_STATIC_SAMPLER_DESC> descs, size_t offset) {
    Assert(offset < _numStaticSamplers);
    size_t i = 0;
    while (i < descs.Size()) {
        if (i + offset < _numStaticSamplers) {
            _staticSamplers[i + offset] = descs[i];
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

void RootSignature::Finalize(Device *pDevice, D3D12_ROOT_SIGNATURE_FLAGS flags) {
    Assert(!_finalized);
    D3D12_ROOT_SIGNATURE_DESC rootDesc = {static_cast<UINT>(_numParameters),
        _rootParameters.data(),
        static_cast<UINT>(_numStaticSamplers),
        _staticSamplers.data(),
        flags};

    // build root Signature
    WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
    WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSig, &errorBlob);

    if (FAILED(hr)) {
        std::string errorMessage(static_cast<const char *>(errorBlob->GetBufferPointer()), errorBlob->GetBufferSize());
        Exception::Throw(errorMessage);
    }

    ThrowIfFailed(pDevice->GetNativeDevice()->CreateRootSignature(0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(&_pRootSignature)));

    for (size_t rootIndex = 0; rootIndex < _numParameters; ++rootIndex) {
        const RootParameter &rootParameter = _rootParameters[rootIndex];
        if (rootParameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
            size_t descriptorTypeIndex = GetPerTableIndexByRangeType(
                rootParameter.DescriptorTable.pDescriptorRanges[0].RangeType);
            for (size_t rangeIndex = 0; rangeIndex < rootParameter.DescriptorTable.NumDescriptorRanges; ++rangeIndex) {
                const D3D12_DESCRIPTOR_RANGE &range = rootParameter.DescriptorTable.pDescriptorRanges[rangeIndex];
                if (range.NumDescriptors <= 0) {
                    continue;
                }
                _descriptorTableBitMap[descriptorTypeIndex].set(rootIndex);
                _numDescriptorPreTable[descriptorTypeIndex][rootIndex] += range.NumDescriptors;
            }
        }
    }

    _finalized = true;
}

auto RootSignature::GetDescriptorTableBitMask(D3D12_DESCRIPTOR_HEAP_TYPE heapType) const -> DescriptorTableBitMask {
    switch (heapType) {
    case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
        return _descriptorTableBitMap[kCBV_UAV_SRV_HeapIndex];
    case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
        return _descriptorTableBitMap[kSAMPLER_HeapIndex];
    default:
        Assert(false);
        return {};
    }
}

auto RootSignature::GetNumDescriptorPreTable(D3D12_DESCRIPTOR_HEAP_TYPE heapType) const
    -> const NumDescriptorPreTable & {
    switch (heapType) {
    case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
        return _numDescriptorPreTable[kCBV_UAV_SRV_HeapIndex];
    case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
        return _numDescriptorPreTable[kSAMPLER_HeapIndex];
    default:
        Assert(false);
        static NumDescriptorPreTable emptyObject;
        return emptyObject;
    }
}

#pragma endregion

}    // namespace dx
