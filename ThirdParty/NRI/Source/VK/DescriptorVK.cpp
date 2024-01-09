// © 2021 NVIDIA Corporation

#include "SharedVK.h"
#include "DescriptorVK.h"
#include "TextureVK.h"
#include "BufferVK.h"

using namespace nri;

DescriptorVK::~DescriptorVK()
{
    const auto& vk = m_Device.GetDispatchTable();

    switch (m_Type)
    {
    case DescriptorTypeVK::NONE:
    case DescriptorTypeVK::ACCELERATION_STRUCTURE:
        break;
    case DescriptorTypeVK::BUFFER_VIEW:
        for (uint32_t i = 0; i < m_Device.GetPhysicalDeviceGroupSize(); i++)
        {
            if (m_BufferViews[i] != VK_NULL_HANDLE)
                vk.DestroyBufferView(m_Device, m_BufferViews[i], m_Device.GetAllocationCallbacks());
        }
        break;
    case DescriptorTypeVK::IMAGE_VIEW:
        for (uint32_t i = 0; i < m_Device.GetPhysicalDeviceGroupSize(); i++)
        {
            if (m_ImageViews[i] != VK_NULL_HANDLE)
                vk.DestroyImageView(m_Device, m_ImageViews[i], m_Device.GetAllocationCallbacks());
        }
        break;
    case DescriptorTypeVK::SAMPLER:
        if (m_Sampler != VK_NULL_HANDLE)
            vk.DestroySampler(m_Device, m_Sampler, m_Device.GetAllocationCallbacks());
        break;
    }
}

template<typename T>
void FillTextureDesc(const T& textureViewDesc, DescriptorTextureDesc& descriptorTextureDesc)
{
    const TextureVK& texture = *(const TextureVK*)textureViewDesc.texture;
    const TextureDesc& textureDesc = texture.GetDesc();

    const Mip_t mipLevelsLeft = textureDesc.mipNum - textureViewDesc.mipOffset;
    const Dim_t arrayLayersLeft = textureDesc.arraySize - descriptorTextureDesc.arrayOffset;

    descriptorTextureDesc.texture = &texture;
    descriptorTextureDesc.layout = GetImageLayoutForView(textureViewDesc.viewType);
    descriptorTextureDesc.aspectFlags = texture.GetImageAspectFlags();
    descriptorTextureDesc.arrayOffset = textureViewDesc.arrayOffset;
    descriptorTextureDesc.arraySize = (textureViewDesc.arraySize == REMAINING_ARRAY_LAYERS) ? arrayLayersLeft : textureViewDesc.arraySize;
    descriptorTextureDesc.mipOffset = textureViewDesc.mipOffset;
    descriptorTextureDesc.mipNum = (textureViewDesc.mipNum == REMAINING_MIP_LEVELS) ? mipLevelsLeft : textureViewDesc.mipNum;
}

template<>
void FillTextureDesc(const Texture3DViewDesc& textureViewDesc, DescriptorTextureDesc& descriptorTextureDesc)
{
    const TextureVK& texture = *(const TextureVK*)textureViewDesc.texture;
    const TextureDesc& textureDesc = texture.GetDesc();

    const Mip_t mipLevelsLeft = textureDesc.mipNum - textureViewDesc.mipOffset;

    descriptorTextureDesc.texture = &texture;
    descriptorTextureDesc.layout = GetImageLayoutForView(textureViewDesc.viewType);
    descriptorTextureDesc.aspectFlags = texture.GetImageAspectFlags();
    descriptorTextureDesc.arrayOffset = 0;
    descriptorTextureDesc.arraySize = 1;
    descriptorTextureDesc.mipOffset = textureViewDesc.mipOffset;
    descriptorTextureDesc.mipNum = (textureViewDesc.mipNum == REMAINING_MIP_LEVELS) ? mipLevelsLeft : textureViewDesc.mipNum;
}

template<typename T>
void FillImageSubresourceRange(const T& textureViewDesc, VkImageSubresourceRange& subresourceRange)
{
    const TextureVK& texture = *(const TextureVK*)textureViewDesc.texture;

    subresourceRange = {
        texture.GetImageAspectFlags(),
        textureViewDesc.mipOffset,
        (textureViewDesc.mipNum == REMAINING_MIP_LEVELS) ? VK_REMAINING_MIP_LEVELS : textureViewDesc.mipNum,
        textureViewDesc.arrayOffset,
        (textureViewDesc.arraySize == REMAINING_ARRAY_LAYERS) ? VK_REMAINING_ARRAY_LAYERS : textureViewDesc.arraySize
    };
}

template<>
void FillImageSubresourceRange(const Texture3DViewDesc& textureViewDesc, VkImageSubresourceRange& subresourceRange)
{
    const TextureVK& texture = *(const TextureVK*)textureViewDesc.texture;

    subresourceRange = {
        texture.GetImageAspectFlags(),
        textureViewDesc.mipOffset,
        (textureViewDesc.mipNum == REMAINING_MIP_LEVELS) ? VK_REMAINING_MIP_LEVELS : textureViewDesc.mipNum,
        0,
        1
    };
}

template<typename T>
Result DescriptorVK::CreateTextureView(const T& textureViewDesc)
{
    const TextureVK& texture = *(const TextureVK*)textureViewDesc.texture;

    VkImageViewUsageCreateInfo imageViewUsageCreateInfo = {};
    imageViewUsageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO;
    imageViewUsageCreateInfo.usage = GetImageViewUsage(textureViewDesc.viewType);

    m_Type = DescriptorTypeVK::IMAGE_VIEW;
    m_Format = ::GetVkImageViewFormat(textureViewDesc.format);
    FillTextureDesc(textureViewDesc, m_TextureDesc);

    VkImageSubresourceRange subresource;
    FillImageSubresourceRange(textureViewDesc, subresource);

    VkImageViewCreateInfo imageViewCreateInfo = {};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.pNext = &imageViewUsageCreateInfo;
    imageViewCreateInfo.viewType = GetImageViewType(textureViewDesc.viewType);
    imageViewCreateInfo.format = m_Format;
    imageViewCreateInfo.subresourceRange = subresource;

    const auto& vk = m_Device.GetDispatchTable();

    const uint32_t nodeMask = GetNodeMask(textureViewDesc.nodeMask);

    for (uint32_t i = 0; i < m_Device.GetPhysicalDeviceGroupSize(); i++)
    {
        if ((1 << i) & nodeMask)
        {
            m_TextureDesc.handles[i] = texture.GetHandle(i);
            imageViewCreateInfo.image = texture.GetHandle(i);

            const VkResult result = vk.CreateImageView(m_Device, &imageViewCreateInfo, m_Device.GetAllocationCallbacks(), &m_ImageViews[i]);

            RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result),
                "Can't create a texture view: vkCreateImageView returned %d.", (int32_t)result);
        }
    }

    return Result::SUCCESS;
}

Result DescriptorVK::Create(const BufferViewDesc& bufferViewDesc)
{
    const BufferVK& buffer = *(const BufferVK*)bufferViewDesc.buffer;

    m_Type = DescriptorTypeVK::BUFFER_VIEW;
    m_Format = GetVkFormat((nri::Format)bufferViewDesc.format);
    m_BufferDesc.offset = bufferViewDesc.offset;
    m_BufferDesc.size = (bufferViewDesc.size == WHOLE_SIZE) ? VK_WHOLE_SIZE : bufferViewDesc.size;

    const uint32_t nodeMask = GetNodeMask(bufferViewDesc.nodeMask);

    for (uint32_t i = 0; i < m_Device.GetPhysicalDeviceGroupSize(); i++)
    {
        if ((1 << i) & nodeMask)
            m_BufferDesc.handles[i] = buffer.GetHandle(i);
    }

    if (bufferViewDesc.format == Format::UNKNOWN)
        return Result::SUCCESS;

    VkBufferViewCreateInfo info = {
        VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
        nullptr,
        (VkBufferViewCreateFlags)0,
        VK_NULL_HANDLE,
        m_Format,
        bufferViewDesc.offset,
        m_BufferDesc.size
    };

    const auto& vk = m_Device.GetDispatchTable();

    for (uint32_t i = 0; i < m_Device.GetPhysicalDeviceGroupSize(); i++)
    {
        if ((1 << i) & nodeMask)
        {
            info.buffer = buffer.GetHandle(i);

            const VkResult result = vk.CreateBufferView(m_Device, &info, m_Device.GetAllocationCallbacks(), &m_BufferViews[i]);

            RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result),
                "Can't create a buffer view: vkCreateBufferView returned %d.", (int32_t)result);
        }
    }

    return Result::SUCCESS;
}

Result DescriptorVK::Create(const Texture1DViewDesc& textureViewDesc)
{
    return CreateTextureView(textureViewDesc);
}

Result DescriptorVK::Create(const Texture2DViewDesc& textureViewDesc)
{
    return CreateTextureView(textureViewDesc);
}

Result DescriptorVK::Create(const Texture3DViewDesc& textureViewDesc)
{
    return CreateTextureView(textureViewDesc);
}

Result DescriptorVK::Create(const SamplerDesc& samplerDesc)
{
    m_Type = DescriptorTypeVK::SAMPLER;

    const VkSamplerCreateInfo samplerInfo = {
        VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        nullptr,
        (VkSamplerCreateFlags)0,
        GetFilter(samplerDesc.filters.mag),
        GetFilter(samplerDesc.filters.min),
        GetSamplerMipmapMode(samplerDesc.filters.mip),
        GetSamplerAddressMode(samplerDesc.addressModes.u),
        GetSamplerAddressMode(samplerDesc.addressModes.v),
        GetSamplerAddressMode(samplerDesc.addressModes.w),
        samplerDesc.mipBias,
        VkBool32(samplerDesc.anisotropy > 1.0f),
        (float)samplerDesc.anisotropy,
        VkBool32(samplerDesc.compareFunc != CompareFunc::NONE),
        GetCompareOp(samplerDesc.compareFunc),
        samplerDesc.mipMin,
        samplerDesc.mipMax,
        VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
        false
    };

    const auto& vk = m_Device.GetDispatchTable();
    const VkResult result = vk.CreateSampler(m_Device, &samplerInfo, m_Device.GetAllocationCallbacks(), &m_Sampler);

    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result),
        "Can't create a sampler: vkCreateSampler returned %d.", (int32_t)result);

    return Result::SUCCESS;
}

Result DescriptorVK::Create(const VkAccelerationStructureKHR* accelerationStructures, uint32_t nodeMask)
{
    m_Type = DescriptorTypeVK::ACCELERATION_STRUCTURE;

    nodeMask = GetNodeMask(nodeMask);

    for (uint32_t i = 0; i < m_Device.GetPhysicalDeviceGroupSize(); i++)
    {
        if ((1 << i) & nodeMask)
            m_AccelerationStructures[i] = accelerationStructures[i];
    }

    return Result::SUCCESS;
}

//================================================================================================================
// NRI
//================================================================================================================

inline void DescriptorVK::SetDebugName(const char* name)
{
    std::array<uint64_t, PHYSICAL_DEVICE_GROUP_MAX_SIZE> handles;

    switch (m_Type)
    {
    case DescriptorTypeVK::BUFFER_VIEW:
        for (size_t i = 0; i < handles.size(); i++)
            handles[i] = (uint64_t)m_BufferViews[i];
        m_Device.SetDebugNameToDeviceGroupObject(VK_OBJECT_TYPE_BUFFER_VIEW, handles.data(), name);
        break;

    case DescriptorTypeVK::IMAGE_VIEW:
        for (size_t i = 0; i < handles.size(); i++)
            handles[i] = (uint64_t)m_ImageViews[i];
        m_Device.SetDebugNameToDeviceGroupObject(VK_OBJECT_TYPE_IMAGE_VIEW, handles.data(), name);
        break;

    case DescriptorTypeVK::SAMPLER:
        m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_SAMPLER, (uint64_t)m_Sampler, name);
        break;

    case DescriptorTypeVK::ACCELERATION_STRUCTURE:
        for (size_t i = 0; i < handles.size(); i++)
            handles[i] = (uint64_t)m_AccelerationStructures[i];
        m_Device.SetDebugNameToDeviceGroupObject(VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, handles.data(), name);
        break;

    default:
        CHECK(&m_Device, false, "unexpected descriptor type in SetDebugName: %u", (uint32_t)m_Type);
        break;
    }
}

#include "DescriptorVK.hpp"