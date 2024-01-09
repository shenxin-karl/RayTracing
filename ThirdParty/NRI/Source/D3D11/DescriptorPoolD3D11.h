// © 2021 NVIDIA Corporation

#pragma once

namespace nri
{

struct DeviceD3D11;

struct DescriptorPoolD3D11
{
    inline DescriptorPoolD3D11(DeviceD3D11& device) :
        m_DescriptorSets(device.GetStdAllocator()),
        m_DescriptorPool(device.GetStdAllocator()),
        m_Device(device)
    {}

    inline ~DescriptorPoolD3D11()
    {}

    inline DeviceD3D11& GetDevice() const
    { return m_Device; }

    Result Create(const DescriptorPoolDesc& descriptorPoolDesc);

    //================================================================================================================
    // NRI
    //================================================================================================================

    inline void SetDebugName(const char* name)
    { MaybeUnused(name); }

    inline void Reset()
    {
        m_DescriptorPoolOffset = 0;
        m_DescriptorSetIndex = 0;
    }

    Result AllocateDescriptorSets(const PipelineLayout& pipelineLayout, uint32_t setIndexInPipelineLayout, DescriptorSet** descriptorSets,
        uint32_t instanceNum, uint32_t nodeMask, uint32_t variableDescriptorNum);

private:
    DeviceD3D11& m_Device;
    Vector<DescriptorSetD3D11> m_DescriptorSets;
    Vector<const DescriptorD3D11*> m_DescriptorPool;
    uint32_t m_DescriptorPoolOffset = 0;
    uint32_t m_DescriptorSetIndex = 0;
};

}
