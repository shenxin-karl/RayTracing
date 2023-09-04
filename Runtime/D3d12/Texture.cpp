#include "Texture.h"

#include "FormatHelper.hpp"
#include "Foundation/StringUtil.h"

namespace dx {

Texture::Texture() : _pResource(nullptr), _textureDesc{}, _formatSupport{} {
    _isSupportRTV = false;
    _isSupportDSV = false;
    _isSupportUAV = false;
    _isSupport2D = false;
    _isSupportCube = false;
    _isSupportLinearSample = false;
    _isSupportMip = false;
    _pAllocation = nullptr;
}

Texture::~Texture() {
    OnDestroy();
}

void Texture::OnCreate(Device *pDevice,
    const D3D12_RESOURCE_DESC &desc,
    D3D12_RESOURCE_STATES initState,
    const D3D12_CLEAR_VALUE *pClearValue) {
}

void Texture::OnDestroy() {
}

void Texture::SetName(std::string_view name) {
    std::wstring wideName = nstd::to_wstring(name);
    _pResource->SetName(wideName.c_str());
}

void Texture::InitFeatureSupport(ID3D12Device *pDevice, DXGI_FORMAT format) {
    auto getFormatSupport = [&](DXGI_FORMAT fmt) {
        D3D12_FEATURE_DATA_FORMAT_SUPPORT support;
        support.Format = fmt;
        ThrowIfFailed(pDevice->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &support, sizeof(support)));
        return support;
    };
    auto checkRTVSupport = [&](const D3D12_FEATURE_DATA_FORMAT_SUPPORT &formatSupport) {
        return _textureDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET &&
               formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_RENDER_TARGET;
    };
    auto checkDSVSupport = [&](const D3D12_FEATURE_DATA_FORMAT_SUPPORT &formatSupport) {
        return _textureDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL &&
               formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL;
    };
    auto checkUASupport = [&](const D3D12_FEATURE_DATA_FORMAT_SUPPORT &formatSupport) {
        // todo: 在不好的显卡上可能会不支持 load 和 store 但是代码可以跑
        return _textureDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS &&
               formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW;    //&&
        //formatSupport.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD				&&
        //formatSupport.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE				;
    };

    _formatSupport = getFormatSupport(format);
    _isSupportRTV = checkRTVSupport(_formatSupport);
    _isSupportDSV = checkDSVSupport(_formatSupport);
    _isSupportUAV = checkUASupport(_formatSupport);
    _isSupport2D = _formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURE2D;
    _isSupportCube = _formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURECUBE;
    _isSupportLinearSample = _formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE;
    _isSupportMip = _formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_MIP;

    if (_textureDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL && IsTypelessFormat(format)) {
        D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupport = getFormatSupport(GetTypelessDepthTextureDSVFormat(format));
        _isSupportDSV = checkDSVSupport(formatSupport);
        _isSupport2D = formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURE2D;
        _isSupportCube = formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURECUBE;
        _isSupportLinearSample = formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE;
        _isSupportMip = formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_MIP;
    }
}

}    // namespace dx
