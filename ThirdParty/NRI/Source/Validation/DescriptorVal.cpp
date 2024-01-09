// © 2021 NVIDIA Corporation

#include "SharedExternal.h"
#include "SharedVal.h"
#include "DescriptorVal.h"

using namespace nri;

DescriptorVal::DescriptorVal(DeviceVal& device, Descriptor* descriptor, ResourceType resourceType) : DeviceObjectVal(device, descriptor), m_ResourceType(resourceType) {
}

DescriptorVal::DescriptorVal(DeviceVal& device, Descriptor* descriptor, const BufferViewDesc& bufferViewDesc)
    : DeviceObjectVal(device, descriptor), m_ResourceType(ResourceType::BUFFER) {
    switch (bufferViewDesc.viewType) {
        case BufferViewType::CONSTANT:
            m_ResourceViewType = ResourceViewType::CONSTANT_BUFFER_VIEW;
            break;
        case BufferViewType::SHADER_RESOURCE:
            m_ResourceViewType = ResourceViewType::SHADER_RESOURCE;
            break;
        case BufferViewType::SHADER_RESOURCE_STORAGE:
            m_ResourceViewType = ResourceViewType::SHADER_RESOURCE_STORAGE;
            break;
        default:
            CHECK(&m_Device, false, "unexpected BufferView type in DescriptorVal: %u", (uint32_t)bufferViewDesc.viewType);
            break;
    }
}

DescriptorVal::DescriptorVal(DeviceVal& device, Descriptor* descriptor, const Texture1DViewDesc& textureViewDesc)
    : DeviceObjectVal(device, descriptor), m_ResourceType(ResourceType::TEXTURE) {
    switch (textureViewDesc.viewType) {
        case Texture1DViewType::SHADER_RESOURCE_1D:
        case Texture1DViewType::SHADER_RESOURCE_1D_ARRAY:
            m_ResourceViewType = ResourceViewType::SHADER_RESOURCE;
            break;
        case Texture1DViewType::SHADER_RESOURCE_STORAGE_1D:
        case Texture1DViewType::SHADER_RESOURCE_STORAGE_1D_ARRAY:
            m_ResourceViewType = ResourceViewType::SHADER_RESOURCE_STORAGE;
            break;
        case Texture1DViewType::COLOR_ATTACHMENT:
            m_ResourceViewType = ResourceViewType::COLOR_ATTACHMENT;
            break;
        case Texture1DViewType::DEPTH_STENCIL_ATTACHMENT:
            m_ResourceViewType = ResourceViewType::DEPTH_STENCIL_ATTACHMENT;
            break;
        default:
            CHECK(&m_Device, false, "unexpected TextureView type in DescriptorVal: %u", (uint32_t)textureViewDesc.viewType);
            break;
    }
}

DescriptorVal::DescriptorVal(DeviceVal& device, Descriptor* descriptor, const Texture2DViewDesc& textureViewDesc)
    : DeviceObjectVal(device, descriptor), m_ResourceType(ResourceType::TEXTURE) {
    switch (textureViewDesc.viewType) {
        case Texture2DViewType::SHADER_RESOURCE_2D:
        case Texture2DViewType::SHADER_RESOURCE_2D_ARRAY:
        case Texture2DViewType::SHADER_RESOURCE_CUBE:
        case Texture2DViewType::SHADER_RESOURCE_CUBE_ARRAY:
            m_ResourceViewType = ResourceViewType::SHADER_RESOURCE;
            break;
        case Texture2DViewType::SHADER_RESOURCE_STORAGE_2D:
        case Texture2DViewType::SHADER_RESOURCE_STORAGE_2D_ARRAY:
            m_ResourceViewType = ResourceViewType::SHADER_RESOURCE_STORAGE;
            break;
        case Texture2DViewType::COLOR_ATTACHMENT:
            m_ResourceViewType = ResourceViewType::COLOR_ATTACHMENT;
            break;
        case Texture2DViewType::DEPTH_STENCIL_ATTACHMENT:
            m_ResourceViewType = ResourceViewType::DEPTH_STENCIL_ATTACHMENT;
            break;
        default:
            CHECK(&m_Device, false, "unexpected TextureView type in DescriptorVal: %u", (uint32_t)textureViewDesc.viewType);
            break;
    }
}

DescriptorVal::DescriptorVal(DeviceVal& device, Descriptor* descriptor, const Texture3DViewDesc& textureViewDesc)
    : DeviceObjectVal(device, descriptor), m_ResourceType(ResourceType::TEXTURE) {
    switch (textureViewDesc.viewType) {
        case Texture3DViewType::SHADER_RESOURCE_3D:
            m_ResourceViewType = ResourceViewType::SHADER_RESOURCE;
            break;
        case Texture3DViewType::SHADER_RESOURCE_STORAGE_3D:
            m_ResourceViewType = ResourceViewType::SHADER_RESOURCE_STORAGE;
            break;
        case Texture3DViewType::COLOR_ATTACHMENT:
            m_ResourceViewType = ResourceViewType::COLOR_ATTACHMENT;
            break;
        default:
            CHECK(&m_Device, false, "unexpected TextureView type in DescriptorVal: %u", (uint32_t)textureViewDesc.viewType);
            break;
    }
}

DescriptorVal::DescriptorVal(DeviceVal& device, Descriptor* descriptor) : DeviceObjectVal(device, descriptor), m_ResourceType(ResourceType::SAMPLER) {
}

void DescriptorVal::SetDebugName(const char* name) {
    m_Name = name;
    GetCoreInterface().SetDescriptorDebugName(*GetImpl(), name);
}

#include "DescriptorVal.hpp"
