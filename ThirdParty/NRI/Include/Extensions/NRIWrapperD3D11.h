// © 2021 NVIDIA Corporation

#pragma once

#include "NRIDeviceCreation.h"

NRI_FORWARD_STRUCT(AGSContext);
NRI_FORWARD_STRUCT(ID3D11Device);
NRI_FORWARD_STRUCT(ID3D11Resource);
NRI_FORWARD_STRUCT(ID3D11DeviceContext);

NRI_NAMESPACE_BEGIN

NRI_STRUCT(DeviceCreationD3D11Desc)
{
    ID3D11Device* d3d11Device;
    AGSContext* agsContext; // can be NULL
    NRI_NAME(CallbackInterface) callbackInterface;
    NRI_NAME(MemoryAllocatorInterface) memoryAllocatorInterface;
    bool enableNRIValidation;
};

NRI_STRUCT(CommandBufferD3D11Desc)
{
    ID3D11DeviceContext* d3d11DeviceContext;
};

NRI_STRUCT(BufferD3D11Desc)
{
    ID3D11Resource* d3d11Resource;
};

NRI_STRUCT(TextureD3D11Desc)
{
    ID3D11Resource* d3d11Resource;
};

NRI_STRUCT(WrapperD3D11Interface)
{
    NRI_NAME(Result) (NRI_CALL *CreateCommandBufferD3D11)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(CommandBufferD3D11Desc) commandBufferD3D11Desc, NRI_NAME_REF(CommandBuffer*) commandBuffer);
    NRI_NAME(Result) (NRI_CALL *CreateBufferD3D11)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(BufferD3D11Desc) bufferD3D11Desc, NRI_NAME_REF(Buffer*) buffer);
    NRI_NAME(Result) (NRI_CALL *CreateTextureD3D11)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(TextureD3D11Desc) textureD3D11Desc, NRI_NAME_REF(Texture*) texture);
};

NRI_API NRI_NAME(Result) NRI_CALL nriCreateDeviceFromD3D11Device(const NRI_NAME_REF(DeviceCreationD3D11Desc) deviceDesc, NRI_NAME_REF(Device*) device);

NRI_NAMESPACE_END
