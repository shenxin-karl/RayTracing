#include "RootSignature.h"
#include "Device.h"
#include <algorithm>

#include "Foundation/StringUtil.h"

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

void RootParameter::InitAsBufferCBV(UINT Register, UINT space, D3D12_SHADER_VISIBILITY visibility) {
    ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    ShaderVisibility = visibility;
    Descriptor.ShaderRegister = Register;
    Descriptor.RegisterSpace = space;
    Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
}

void RootParameter::InitAsBufferSRV(UINT Register, UINT space, D3D12_SHADER_VISIBILITY visibility) {
    ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
    ShaderVisibility = visibility;
    Descriptor.ShaderRegister = Register;
    Descriptor.RegisterSpace = space;
    Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
}

void RootParameter::InitAsBufferUAV(UINT Register, UINT space, D3D12_SHADER_VISIBILITY visibility) {
    ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
    ShaderVisibility = visibility;
    Descriptor.ShaderRegister = Register;
    Descriptor.RegisterSpace = space;
    Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
}

void RootParameter::InitAsDescriptorTable(UINT rangeCount, D3D12_SHADER_VISIBILITY visibility) {
    ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    ShaderVisibility = visibility;
    DescriptorTable.NumDescriptorRanges = rangeCount;
    DescriptorTable.pDescriptorRanges = new D3D12_DESCRIPTOR_RANGE1[rangeCount];
}

void RootParameter::InitAsDescriptorTable(std::initializer_list<D3D12_DESCRIPTOR_RANGE1> ranges,
    D3D12_SHADER_VISIBILITY visibility) {

    ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    ShaderVisibility = visibility;
    DescriptorTable.NumDescriptorRanges = ranges.size();
    DescriptorTable.pDescriptorRanges = new D3D12_DESCRIPTOR_RANGE1[ranges.size()];
    size_t index = 0;
    for (const D3D12_DESCRIPTOR_RANGE1 &range : ranges) {
        const D3D12_DESCRIPTOR_RANGE1 &dest = DescriptorTable.pDescriptorRanges[index++];
        const_cast<D3D12_DESCRIPTOR_RANGE1 &>(dest) = range;
    }
}

void RootParameter::SetTableRange(size_t rangeIndex,
    D3D12_DESCRIPTOR_RANGE_TYPE type,
    UINT Register,
    UINT count,
    UINT space,
    D3D12_DESCRIPTOR_RANGE_FLAGS Flags) {

    D3D12_DESCRIPTOR_RANGE1 *pRange = const_cast<D3D12_DESCRIPTOR_RANGE1 *>(
        DescriptorTable.pDescriptorRanges + rangeIndex);
    pRange->RangeType = type;
    pRange->NumDescriptors = count;
    pRange->BaseShaderRegister = Register;
    pRange->RegisterSpace = space;
    pRange->Flags = Flags;
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

RootSignature::RootSignature(size_t numRootParam, size_t numStaticSamplers) {
     _numParameters = numRootParam;
    _numStaticSamplers = numStaticSamplers;
    for (size_t i = 0; i < 2; ++i) {
        std::ranges::fill(_descriptorTableInfo[i], DescriptorTableInfo{0, false});
        _descriptorTableBitMap[i].reset();
    }
    _pRootSignature = nullptr;
    _rootParameters.resize(numRootParam);
    _staticSamplers.resize(numStaticSamplers);
}

RootSignature::~RootSignature() {
    _numParameters = 0;
    _pRootSignature = nullptr;
    _rootParameters.clear();
    _staticSamplers.clear();
}

void RootSignature::SetStaticSamplers(ReadonlyArraySpan<CD3DX12_STATIC_SAMPLER_DESC> descs, size_t offset) {
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

void RootSignature::Generate(Device *pDevice, D3D12_ROOT_SIGNATURE_FLAGS flags) {
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootDesc;
    rootDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    rootDesc.Desc_1_1 = D3D12_ROOT_SIGNATURE_DESC1{
        static_cast<UINT>(_numParameters),
        _rootParameters.data(),
        static_cast<UINT>(_numStaticSamplers),
        _staticSamplers.data(),
        flags,
    };

    // build root Signature
    WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
    WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeVersionedRootSignature(&rootDesc, &serializedRootSig, &errorBlob);

    if (FAILED(hr)) {
        std::string errorMessage(static_cast<const char *>(errorBlob->GetBufferPointer()), errorBlob->GetBufferSize());
        Exception::Throw(errorMessage);
    }

    ThrowIfFailed(pDevice->GetNativeDevice()->CreateRootSignature(0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(&_pRootSignature)));

    // collect descriptor range info
    for (size_t rootIndex = 0; rootIndex < _numParameters; ++rootIndex) {
        const RootParameter &rootParameter = _rootParameters[rootIndex];
        if (rootParameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
            size_t descriptorTypeIndex = GetPerTableIndexByRangeType(
                rootParameter.DescriptorTable.pDescriptorRanges[0].RangeType);
            for (size_t rangeIndex = 0; rangeIndex < rootParameter.DescriptorTable.NumDescriptorRanges; ++rangeIndex) {
                const D3D12_DESCRIPTOR_RANGE1 &range = rootParameter.DescriptorTable.pDescriptorRanges[rangeIndex];
                if (range.NumDescriptors <= 0) {
                    continue;
                }
                // enable bindless
                _descriptorTableBitMap[descriptorTypeIndex].set(rootIndex);
                if (range.Flags & D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE) {
                    _descriptorTableInfo[descriptorTypeIndex][rootIndex].enableBindless = true;
                } else {
                    _descriptorTableInfo[descriptorTypeIndex][rootIndex].numDescriptor += range.NumDescriptors;
                }
            }
        }
    }

    if (!_name.empty()) {
	    std::wstring wstring = nstd::to_wstring(_name);
		_pRootSignature->SetName(wstring.c_str());	
    }
}

auto RootSignature::GetRootParamDescriptorTableInfo(D3D12_DESCRIPTOR_HEAP_TYPE heapType) const
    -> const RootParamDescriptorTableInfo & {

    switch (heapType) {
    case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
        return _descriptorTableInfo[0];
    case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
        return _descriptorTableInfo[1];
    default:
        Exception::Throw("Invalid HeapType: {}", heapType);
        return _descriptorTableInfo[0];
    }
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

void RootSignature::SetName(std::string_view name) {
    _name = name;
    if (_pRootSignature != nullptr) {
        std::wstring wstring = nstd::to_wstring(name);
		_pRootSignature->SetName(wstring.c_str());	    
    }
}

#pragma endregion

}    // namespace dx
