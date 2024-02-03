#pragma once
#include "D3dStd.h"
#include "Foundation/NamespeceAlias.h"
#include "Foundation/Memory/SharedPtr.hpp"
#include <string>

namespace dx {

class Texture : public RefCounter  {
private:
    Texture(Device *pDevice,
        const D3D12_RESOURCE_DESC &desc,
        D3D12_RESOURCE_STATES initState,
        const D3D12_CLEAR_VALUE *pClearValue = nullptr);
public:
    template<typename ...Args>
	static SharedPtr<Texture> Create(Args &&...args) {
	    return SharedPtr<Texture>(new Texture(std::forward<Args>(args)...));
    }

    ~Texture() override;
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
    auto GetFlags() const -> D3D12_RESOURCE_FLAGS {
	    return _textureDesc.Flags;
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
