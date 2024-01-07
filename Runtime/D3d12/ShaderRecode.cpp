#include "ShaderRecode.h"
#include "DescriptorHandleArray.hpp"
#include "RootSignature.h"

namespace dx {

LocalRootParameterData::LocalRootParameterData(const RootSignature *pRootSignature) : _pRootSignature(pRootSignature) {
    if (_pRootSignature != nullptr) {
        InitLayout(_pRootSignature);
    }
}

void LocalRootParameterData::InitLayout(const RootSignature *pRootSignature) {
    Assert(pRootSignature != nullptr);
    _pRootSignature = pRootSignature;
    _rootParamDataList.resize(_pRootSignature->GetRootParameters().size());
    const std::vector<RootParameter> &rootParameters = _pRootSignature->GetRootParameters();
    for (size_t i = 0; i < rootParameters.size(); ++i) {
        const RootParameter &rootParameter = rootParameters[i];
        switch (rootParameter.ParameterType) {
        case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS: {
            size_t num32BitValue = rootParameter.Constants.Num32BitValues;
            _rootParamDataList[i].emplace<std::vector<DWParam>>(num32BitValue, DWParam::Zero);
            break;
        }
        case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE: {
            for (size_t rangeIdx = 0; rangeIdx != rootParameter.DescriptorTable.NumDescriptorRanges; ++rangeIdx) {
                const D3D12_DESCRIPTOR_RANGE1 &range = rootParameter.DescriptorTable.pDescriptorRanges[rangeIdx];
                Assert(range.RangeType != D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER);
            }
            break;
        }
        default:
            break;
        }
    }
}

void LocalRootParameterData::SetConstants(size_t rootIndex, ReadonlyArraySpan<DWParam> constants, size_t offset) {
    Assert(rootIndex < _rootParamDataList.size());
    Assert(_rootParamDataList[rootIndex].index() == eConstants);
    std::vector<DWParam> &rootConstants = std::get<eConstants>(_rootParamDataList[rootIndex]);
    assert(constants.Count() + offset <= rootConstants.size());
    for (size_t i = 0; i < constants.Count(); ++i) {
        rootConstants[i + offset] = constants[i];
    }
}

void LocalRootParameterData::SetConstants(size_t rootIndex,
    size_t num32BitValuesToSet,
    const void *pData,
    size_t offset) {

    Assert(rootIndex < _rootParamDataList.size());
    Assert(_rootParamDataList[rootIndex].index() == eConstants);
    std::vector<DWParam> &rootConstants = std::get<eConstants>(_rootParamDataList[rootIndex]);
    assert(num32BitValuesToSet + offset <= rootConstants.size());

    float *pFloat = (float *)pData;
    for (size_t i = 0; i < num32BitValuesToSet; ++i) {
        rootConstants[i + offset] = *pFloat;
        ++pFloat;
    }
}

void LocalRootParameterData::SetView(size_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferLocation) {
    Assert(rootIndex < _rootParamDataList.size());
    Assert(GetRootParamType(rootIndex) == eView);
    _rootParamDataList[rootIndex] = bufferLocation;
}

void LocalRootParameterData::SetDescriptorTable(size_t rootIndex, std::shared_ptr<DescriptorHandleArray> pHandleArray) {
    Assert(rootIndex < _rootParamDataList.size());
    Assert(GetRootParamType(rootIndex) == eDescriptorTable);
    const auto &tableInfo = _pRootSignature->GetRootParamDescriptorTableInfo(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    if (!tableInfo[rootIndex].enableBindless && pHandleArray->Count() > tableInfo[rootIndex].numDescriptor) {
        Exception::Throw("Invalid Descriptor Table out of range!");
    }
    _rootParamDataList[rootIndex] = std::move(pHandleArray);
}

auto LocalRootParameterData::GetSize() const -> size_t {
    if (_pRootSignature == nullptr) {
        return 0;
    }
    size_t size = 0;
    const std::vector<RootParameter> &rootParameters = _pRootSignature->GetRootParameters();
    for (const RootParameter &rootParam : rootParameters) {
        switch (rootParam.ParameterType) {
        case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
            size = AlignUp(size, sizeof(D3D12_GPU_DESCRIPTOR_HANDLE)) + sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
            break;
        case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
            size = AlignUp(size, sizeof(DWParam)) + sizeof(DWParam) * rootParam.Constants.Num32BitValues;
            break;
        case D3D12_ROOT_PARAMETER_TYPE_CBV:
        case D3D12_ROOT_PARAMETER_TYPE_SRV:
        case D3D12_ROOT_PARAMETER_TYPE_UAV:
            size = AlignUp(size, sizeof(D3D12_GPU_VIRTUAL_ADDRESS)) + sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
            break;
        default:
            Assert(false);
        }
    }
    return size;
}

auto LocalRootParameterData::GetRootParamType(size_t rootIndex) const -> size_t {
    const std::vector<RootParameter> &rootParameters = _pRootSignature->GetRootParameters();
    switch (rootParameters[rootIndex].ParameterType) {
    case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
        return eDescriptorTable;
    case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
        return eConstants;
    case D3D12_ROOT_PARAMETER_TYPE_CBV:
    case D3D12_ROOT_PARAMETER_TYPE_SRV:
    case D3D12_ROOT_PARAMETER_TYPE_UAV:
        return eView;
    default:
        return -1;
    }
}

ShaderRecode::ShaderRecode(void *pShaderIdentifier) : _pShaderIdentifier(pShaderIdentifier) {
}

ShaderRecode::ShaderRecode(void *pShaderIdentifier, const RootSignature *pLocalRootSignature)
    : _pShaderIdentifier(pShaderIdentifier) {
    _localRootParameterData.InitLayout(pLocalRootSignature);
}

ShaderRecode::ShaderRecode(void *pShaderIdentifier, LocalRootParameterData localRootParameterData)
    : _pShaderIdentifier(pShaderIdentifier), _localRootParameterData(std::move(localRootParameterData)) {
}

}    // namespace dx
