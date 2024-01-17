#pragma once
#include "D3dStd.h"
#include "Foundation/NamespeceAlias.h"
#include <source_location>
#include <string>

namespace dx {

class Texture : NonCopyable {
public:
    Texture(const std::source_location &sl = std::source_location::current());
    explicit Texture(std::string_view name);
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
    // clang-format off
    std::string                         _name;
    Device *                            _pDevice;
    WRL::ComPtr<ID3D12Resource>         _pResource;
    D3D12MA::Allocation *               _pAllocation;
    D3D12_RESOURCE_DESC                 _textureDesc;
    // clang-format on
};

}    // namespace dx
