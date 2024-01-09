// © 2021 NVIDIA Corporation

#pragma once

#include "SharedD3D12.h"

struct ID3D12Resource;

namespace nri
{

struct DeviceD3D12;
struct MemoryD3D12;

struct TextureD3D12
{
    inline TextureD3D12(DeviceD3D12& device)
        : m_Device(device)
    {}

    inline ~TextureD3D12()
    {}

    inline DeviceD3D12& GetDevice() const
    { return m_Device; }

    inline const TextureDesc& GetDesc() const
    { return m_Desc; }

    inline operator ID3D12Resource*() const
    { return m_Texture.GetInterface(); }

    inline uint32_t GetSubresourceIndex(Dim_t arrayOffset, Mip_t mipOffset) const
    { return arrayOffset * m_Desc.mipNum + mipOffset; }

    Result Create(const TextureDesc& textureDesc);
    Result Create(const TextureD3D12Desc& textureDesc);
    Result BindMemory(const MemoryD3D12* memory, uint64_t offset);
    Dim_t GetSize(Dim_t dimensionIndex, Mip_t mip = 0) const;

    //================================================================================================================
    // NRI
    //================================================================================================================

    inline void SetDebugName(const char* name)
    { SET_D3D_DEBUG_OBJECT_NAME(m_Texture, name); }

    void GetMemoryInfo(MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const;

private:
    DeviceD3D12& m_Device;
    TextureDesc m_Desc = {};
    ComPtr<ID3D12Resource> m_Texture;
};

}
