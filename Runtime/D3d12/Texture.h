#pragma once
#include "D3dUtils.h"

namespace dx {

class Texture : public NonCopyable {
public:
    Texture();
    ~Texture();
public:
    void OnCreate(Device *pDevice,
        const D3D12_RESOURCE_DESC &desc,
        D3D12_RESOURCE_STATES initState,
        const D3D12_CLEAR_VALUE *pClearValue = nullptr);
    void OnDestroy();
    void SetName(std::string_view name);

    auto GetWidth() const -> uint64_t {
        return _textureDesc.Width;
    }
    auto GetHeight() const -> uint64_t {
        return _textureDesc.Height;
    }
    auto GetFormat() const -> DXGI_FORMAT {
        return _textureDesc.Format;
    }
    auto GetMipCount() const -> uint16_t {
        return _textureDesc.MipLevels;
    }
    auto GetDepthOrArraySize() const -> uint16_t {
        return _textureDesc.DepthOrArraySize;
    }
    auto GetResource() const -> ID3D12Resource * {
        return _pResource.Get();
    }
private:
	void InitFeatureSupport(ID3D12Device *pDevice, DXGI_FORMAT format);
private:
    // clang-format off
    bool                                _isSupportRTV;
	bool                                _isSupportDSV;
	bool                                _isSupportUAV;
	bool                                _isSupport2D;
	bool                                _isSupportCube;
	bool                                _isSupportLinearSample;
	bool                                _isSupportMip;
    WRL::ComPtr<ID3D12Resource>         _pResource;
    D3D12MA::Allocation *               _pAllocation;
    D3D12_RESOURCE_DESC                 _textureDesc;
    D3D12_FEATURE_DATA_FORMAT_SUPPORT   _formatSupport;

    // clang-format on
};

}    // namespace dx
