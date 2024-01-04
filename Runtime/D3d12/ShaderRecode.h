#pragma once
#include "D3dStd.h"
#include "Foundation/ReadonlyArraySpan.hpp"

namespace dx {

class LocalRootParameterData {
public:
    LocalRootParameterData(const RootSignature *pRootSignature = nullptr);
    LocalRootParameterData(LocalRootParameterData &&other) noexcept
        : _pRootSignature(std::exchange(other._pRootSignature, nullptr)),
          _rootParamDataList(std::move(other._rootParamDataList)) {
    }
    LocalRootParameterData &operator=(LocalRootParameterData &&other) noexcept {
        if (this == &other) {
            return *this;
        }
        _pRootSignature = std::exchange(other._pRootSignature, nullptr);
        _rootParamDataList = std::move(other._rootParamDataList);
        return *this;
    }
    friend void swap(LocalRootParameterData &lhs, LocalRootParameterData &rhs) noexcept {
        using std::swap;
        swap(lhs._pRootSignature, rhs._pRootSignature);
        swap(lhs._rootParamDataList, rhs._rootParamDataList);
    }
public:
    void InitLayout(const RootSignature *pRootSignature);
    void SetConstants(size_t rootIndex, ReadonlyArraySpan<DWParam> constants, size_t offset = 0);
    void SetConstants(size_t rootIndex, size_t num32BitValuesToSet, const void *pData, size_t offset = 0);
    void SetView(size_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferLocation);
    void SetDescriptorTable(size_t rootIndex, std::shared_ptr<DescriptorHandleArray> pHandleArray);
    auto GetSize() const -> size_t;
private:
    friend class ComputeContext;
    enum { eConstants, eView, eDescriptorTable };
    auto GetRootParamType(size_t rootIndex) const -> size_t;
    using RootParamData = std::
        variant<std::vector<DWParam>, D3D12_GPU_VIRTUAL_ADDRESS, std::shared_ptr<DescriptorHandleArray>>;
private:
    // clang-format off
    const RootSignature        *_pRootSignature;
    std::vector<RootParamData>  _rootParamDataList;
    // clang-format on
};

class ShaderRecode {
public:
    ShaderRecode(void *pShaderIdentifier = nullptr);
    ShaderRecode(void *pShaderIdentifier, const RootSignature *pLocalRootSignature);
    ShaderRecode(void *pShaderIdentifier, LocalRootParameterData localRootParameterData);
public:
    auto GetSize() const -> size_t {
        return AlignUp<size_t>(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + _localRootParameterData.GetSize(),
            D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    }
    auto GetLocalRootParameterData() -> LocalRootParameterData & {
        return _localRootParameterData;
    }
    auto GetLocalRootParameterData() const -> const LocalRootParameterData & {
        return _localRootParameterData;
    }
    auto GetShaderIdentifier() const -> void * {
	    return _pShaderIdentifier;
    }
    void SetShaderIdentifier(void *pShaderIdentifier) {
	    _pShaderIdentifier = pShaderIdentifier;
    }
    constexpr static inline size_t kAddressAlignment = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES * 2;
private:
    // clang-format off
    void                    *_pShaderIdentifier;
    LocalRootParameterData   _localRootParameterData;
    // clang-format on
};

}    // namespace dx
