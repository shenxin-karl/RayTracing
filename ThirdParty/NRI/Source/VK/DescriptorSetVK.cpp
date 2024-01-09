// © 2021 NVIDIA Corporation

#include "SharedVK.h"
#include "DescriptorSetVK.h"
#include "DescriptorVK.h"

using namespace nri;

struct SlabAllocator
{
    inline SlabAllocator(void* memory, size_t size) :
        m_CurrentOffset((uint8_t*)memory),
        m_End((size_t)memory + size),
        m_Memory((uint8_t*)memory)
    {}

    template<typename T>
    inline T* Allocate(uint32_t& number)
    {
        T* items = (T*)Align(m_CurrentOffset, alignof(T));
        const size_t itemsLeft = (m_End - (size_t)items) / sizeof(T);
        number = std::min<uint32_t>((uint32_t)itemsLeft, number);
        m_CurrentOffset = (uint8_t*)(items + number);
        return items;
    }

    inline void Reset()
    { m_CurrentOffset = m_Memory; }

private:
    uint8_t * m_CurrentOffset;
    size_t m_End;
    uint8_t* m_Memory;
};

static bool WriteTextures(uint32_t nodeIndex, const DescriptorRangeDesc& rangeDesc, const DescriptorRangeUpdateDesc& update,
    uint32_t& descriptorOffset, VkWriteDescriptorSet& write, SlabAllocator& slab)
{
    const uint32_t totalItemNum = update.descriptorNum - descriptorOffset;
    uint32_t itemNumForWriting = totalItemNum;
    VkDescriptorImageInfo* infoArray = slab.Allocate<VkDescriptorImageInfo>(itemNumForWriting);

    for (uint32_t i = 0; i < itemNumForWriting; i++)
    {
        const DescriptorVK& descriptorImpl = *(DescriptorVK*)update.descriptors[descriptorOffset + i];

        infoArray[i].imageView = descriptorImpl.GetImageView(nodeIndex);
        infoArray[i].imageLayout = descriptorImpl.GetImageLayout();
        infoArray[i].sampler = VK_NULL_HANDLE;
    }

    write.descriptorType = GetDescriptorType(rangeDesc.descriptorType);
    write.pImageInfo = infoArray;
    write.descriptorCount = itemNumForWriting;

    descriptorOffset += itemNumForWriting;
    return itemNumForWriting == totalItemNum;
}

static bool WriteTypedBuffers(uint32_t nodeIndex, const DescriptorRangeDesc& rangeDesc, const DescriptorRangeUpdateDesc& update,
    uint32_t& descriptorOffset, VkWriteDescriptorSet& write, SlabAllocator& slab)
{
    const uint32_t totalItemNum = update.descriptorNum - descriptorOffset;
    uint32_t itemNumForWriting = totalItemNum;
    VkBufferView* viewArray = slab.Allocate<VkBufferView>(itemNumForWriting);

    for (uint32_t i = 0; i < itemNumForWriting; i++)
    {
        const DescriptorVK& descriptorImpl = *(DescriptorVK*)update.descriptors[descriptorOffset + i];
        viewArray[i] = descriptorImpl.GetBufferView(nodeIndex);
    }

    write.descriptorType = GetDescriptorType(rangeDesc.descriptorType);
    write.pTexelBufferView = viewArray;
    write.descriptorCount = itemNumForWriting;

    descriptorOffset += itemNumForWriting;
    return itemNumForWriting == totalItemNum;
}

static bool WriteBuffers(uint32_t nodeIndex, const DescriptorRangeDesc& rangeDesc, const DescriptorRangeUpdateDesc& update,
    uint32_t& descriptorOffset, VkWriteDescriptorSet& write, SlabAllocator& slab)
{
    const uint32_t totalItemNum = update.descriptorNum - descriptorOffset;
    uint32_t itemNumForWriting = totalItemNum;
    VkDescriptorBufferInfo* infoArray = slab.Allocate<VkDescriptorBufferInfo>(itemNumForWriting);

    for (uint32_t i = 0; i < itemNumForWriting; i++)
    {
        const DescriptorVK& descriptor = *(DescriptorVK*)update.descriptors[descriptorOffset + i];
        descriptor.GetBufferInfo(nodeIndex, infoArray[i]);
    }

    write.descriptorType = GetDescriptorType(rangeDesc.descriptorType);
    write.pBufferInfo = infoArray;
    write.descriptorCount = itemNumForWriting;

    descriptorOffset += itemNumForWriting;
    return itemNumForWriting == totalItemNum;
}

static bool WriteSamplers(uint32_t nodeIndex, const DescriptorRangeDesc& rangeDesc, const DescriptorRangeUpdateDesc& update,
    uint32_t& descriptorOffset, VkWriteDescriptorSet& write, SlabAllocator& slab)
{
    MaybeUnused(rangeDesc);
    MaybeUnused(nodeIndex);

    const uint32_t totalItemNum = update.descriptorNum - descriptorOffset;
    uint32_t itemNumForWriting = totalItemNum;
    VkDescriptorImageInfo* infoArray = slab.Allocate<VkDescriptorImageInfo>(itemNumForWriting);

    for (uint32_t i = 0; i < itemNumForWriting; i++)
    {
        const DescriptorVK& descriptorImpl = *(DescriptorVK*)update.descriptors[descriptorOffset + i];
        infoArray[i].imageView = VK_NULL_HANDLE;
        infoArray[i].imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        infoArray[i].sampler = descriptorImpl.GetSampler();
    }

    write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    write.pImageInfo = infoArray;
    write.descriptorCount = itemNumForWriting;

    descriptorOffset += itemNumForWriting;
    return itemNumForWriting == totalItemNum;
}

static bool WriteAccelerationStructures(uint32_t nodeIndex, const DescriptorRangeDesc& rangeDesc, const DescriptorRangeUpdateDesc& update,
    uint32_t& descriptorOffset, VkWriteDescriptorSet& write, SlabAllocator& slab)
{
    MaybeUnused(rangeDesc);

    const uint32_t totalItemNum = update.descriptorNum - descriptorOffset;
    uint32_t itemNumForWriting = totalItemNum;

    uint32_t infoNum = 1;
    VkWriteDescriptorSetAccelerationStructureKHR* info = slab.Allocate<VkWriteDescriptorSetAccelerationStructureKHR>(infoNum);

    if (infoNum != 1)
        return false;

    VkAccelerationStructureKHR* handles = slab.Allocate<VkAccelerationStructureKHR>(itemNumForWriting);

    info->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    info->pNext = nullptr;
    info->accelerationStructureCount = itemNumForWriting;
    info->pAccelerationStructures = handles;

    for (uint32_t i = 0; i < itemNumForWriting; i++)
    {
        const DescriptorVK& descriptorImpl = *(DescriptorVK*)update.descriptors[descriptorOffset + i];
        handles[i] = descriptorImpl.GetAccelerationStructure(nodeIndex);
    }

    write.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    write.pNext = info;
    write.descriptorCount = itemNumForWriting;

    descriptorOffset += itemNumForWriting;
    return itemNumForWriting == totalItemNum;
}

typedef bool(*WriteDescriptorsFunc)(uint32_t nodeIndex, const DescriptorRangeDesc& rangeDesc, const DescriptorRangeUpdateDesc& update,
    uint32_t& descriptorOffset, VkWriteDescriptorSet& write, SlabAllocator& slab);

constexpr std::array<WriteDescriptorsFunc, (uint32_t)DescriptorType::MAX_NUM> WRITE_FUNCS =
{
    (WriteDescriptorsFunc)&WriteSamplers,                   // SAMPLER
    (WriteDescriptorsFunc)&WriteBuffers,                    // CONSTANT_BUFFER
    (WriteDescriptorsFunc)&WriteTextures,                   // TEXTURE
    (WriteDescriptorsFunc)&WriteTextures,                   // STORAGE_TEXTURE
    (WriteDescriptorsFunc)&WriteTypedBuffers,               // BUFFER
    (WriteDescriptorsFunc)&WriteTypedBuffers,               // STORAGE_BUFFER
    (WriteDescriptorsFunc)&WriteBuffers,                    // STRUCTURED_BUFFER
    (WriteDescriptorsFunc)&WriteBuffers,                    // STORAGE_STRUCTURED_BUFFER
    (WriteDescriptorsFunc)&WriteAccelerationStructures,     // ACCELERATION_STRUCTURE
};

void DescriptorSetVK::Create(const VkDescriptorSet* handles, uint32_t nodeMask, const DescriptorSetDesc& setDesc)
{
    m_Desc = &setDesc;
    m_DynamicConstantBufferNum = setDesc.dynamicConstantBufferNum;

    uint32_t handleIndex = 0;
    for (uint32_t i = 0; i < m_Device.GetPhysicalDeviceGroupSize(); i++)
    {
        if ((1 << i) & nodeMask)
            m_Handles[i] = handles[handleIndex++];
    }
}

//================================================================================================================
// NRI
//================================================================================================================

inline void DescriptorSetVK::SetDebugName(const char* name)
{
    std::array<uint64_t, PHYSICAL_DEVICE_GROUP_MAX_SIZE> handles;
    for (size_t i = 0; i < handles.size(); i++)
        handles[i] = (uint64_t)m_Handles[i];

    m_Device.SetDebugNameToDeviceGroupObject(VK_OBJECT_TYPE_DESCRIPTOR_SET, handles.data(), name);
}

inline void DescriptorSetVK::UpdateDescriptorRanges(uint32_t nodeMask, uint32_t rangeOffset, uint32_t rangeNum, const DescriptorRangeUpdateDesc* rangeUpdateDescs)
{
    constexpr uint32_t writesPerIteration = 1024;
    uint32_t writeMaxNum = std::min<uint32_t>(writesPerIteration, rangeNum);

    VkWriteDescriptorSet* writes = STACK_ALLOC(VkWriteDescriptorSet, writeMaxNum);

    constexpr size_t slabSize = 32768;
    SlabAllocator slab(STACK_ALLOC(uint8_t, slabSize), slabSize);

    const auto& vk = m_Device.GetDispatchTable();

    nodeMask = GetNodeMask(nodeMask);

    for (uint32_t i = 0; i < m_Device.GetPhysicalDeviceGroupSize(); i++)
    {
        if (!((1 << i) & nodeMask))
            continue;

        uint32_t j = 0;
        uint32_t descriptorOffset = 0;

        do
        {
            slab.Reset();
            bool slabHasFreeSpace = true;
            uint32_t writeNum = 0;

            while (slabHasFreeSpace && j < rangeNum && writeNum < writeMaxNum)
            {
                const DescriptorRangeUpdateDesc& update = rangeUpdateDescs[j];
                const DescriptorRangeDesc& rangeDesc = m_Desc->ranges[rangeOffset + j];

                const uint32_t bindingOffset = rangeDesc.isArray ? 0 : update.offsetInRange + descriptorOffset;
                const uint32_t arrayElement = rangeDesc.isArray ? update.offsetInRange + descriptorOffset : 0;

                VkWriteDescriptorSet& write = writes[writeNum++];
                write = {};
                write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                write.dstSet = m_Handles[i];
                write.dstBinding = rangeDesc.baseRegisterIndex + bindingOffset;
                write.dstArrayElement = arrayElement;

                const uint32_t index = (uint32_t)rangeDesc.descriptorType;
                slabHasFreeSpace = WRITE_FUNCS[index](i, rangeDesc, update, descriptorOffset, write, slab);

                j += (descriptorOffset == update.descriptorNum) ? 1 : 0;
                descriptorOffset = (descriptorOffset == update.descriptorNum) ? 0 : descriptorOffset;
            }

            vk.UpdateDescriptorSets(m_Device, writeNum, writes, 0, nullptr);
        }
        while (j < rangeNum);
    }
}

inline void DescriptorSetVK::UpdateDynamicConstantBuffers(uint32_t nodeMask, uint32_t bufferOffset, uint32_t descriptorNum, const Descriptor* const* descriptors)
{
    const uint32_t descriptorMaxNum = descriptorNum * m_Device.GetPhysicalDeviceGroupSize();

    VkWriteDescriptorSet* writes = STACK_ALLOC(VkWriteDescriptorSet, descriptorMaxNum);
    VkDescriptorBufferInfo* infos = STACK_ALLOC(VkDescriptorBufferInfo, descriptorMaxNum);
    uint32_t writeNum = 0;

    nodeMask = GetNodeMask(nodeMask);

    for (uint32_t i = 0; i < m_Device.GetPhysicalDeviceGroupSize(); i++)
    {
        if (!((1 << i) & nodeMask))
            continue;

        for (uint32_t j = 0; j < descriptorNum; j++)
        {
            const DynamicConstantBufferDesc& bufferDesc = m_Desc->dynamicConstantBuffers[bufferOffset + j];

            VkDescriptorBufferInfo& bufferInfo = infos[writeNum];
            const DescriptorVK& descriptorImpl = *(const DescriptorVK*)descriptors[j];
            descriptorImpl.GetBufferInfo(i, bufferInfo);

            writes[writeNum++] = {
                VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                nullptr,
                m_Handles[i],
                bufferDesc.registerIndex,
                0,
                1,
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                nullptr,
                &bufferInfo
            };
        }
    }

    const auto& vk = m_Device.GetDispatchTable();
    vk.UpdateDescriptorSets(m_Device, writeNum, writes, 0, nullptr);
}

inline void DescriptorSetVK::Copy(const DescriptorSetCopyDesc& descriptorSetCopyDesc)
{
    const uint32_t descriptorRangeNum = descriptorSetCopyDesc.rangeNum + descriptorSetCopyDesc.dynamicConstantBufferNum;
    const uint32_t copyMaxNum = descriptorRangeNum * m_Device.GetPhysicalDeviceGroupSize();

    VkCopyDescriptorSet* copies = STACK_ALLOC(VkCopyDescriptorSet, copyMaxNum);
    uint32_t copyNum = 0;

    const DescriptorSetVK& srcSetImpl = *(const DescriptorSetVK*)descriptorSetCopyDesc.srcDescriptorSet;

    const uint32_t nodeMask = GetNodeMask(descriptorSetCopyDesc.nodeMask);

    for (uint32_t i = 0; i < m_Device.GetPhysicalDeviceGroupSize(); i++)
    {
        if (!((1 << i) & nodeMask))
            continue;

        for (uint32_t j = 0; j < descriptorSetCopyDesc.rangeNum; j++)
        {
            const DescriptorRangeDesc& srcRangeDesc = srcSetImpl.m_Desc->ranges[descriptorSetCopyDesc.baseSrcRange + j];
            const DescriptorRangeDesc& dstRangeDesc = m_Desc->ranges[descriptorSetCopyDesc.baseDstRange + j];

            copies[copyNum++] = {
                VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET,
                nullptr,
                srcSetImpl.GetHandle(i),
                srcRangeDesc.baseRegisterIndex,
                0,
                m_Handles[i],
                dstRangeDesc.baseRegisterIndex,
                0,
                dstRangeDesc.descriptorNum
            };
        }

        for (uint32_t j = 0; j < descriptorSetCopyDesc.dynamicConstantBufferNum; j++)
        {
            const uint32_t srcBufferIndex = descriptorSetCopyDesc.baseSrcDynamicConstantBuffer + j;
            const DynamicConstantBufferDesc& srcBuffer = srcSetImpl.m_Desc->dynamicConstantBuffers[srcBufferIndex];
            const DynamicConstantBufferDesc& dstBuffer = m_Desc->dynamicConstantBuffers[srcBufferIndex];

            copies[copyNum++] = {
                VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET,
                nullptr,
                srcSetImpl.GetHandle(i),
                srcBuffer.registerIndex,
                0,
                m_Handles[i],
                dstBuffer.registerIndex,
                0,
                1
            };
        }
    }

    const auto& vk = m_Device.GetDispatchTable();
    vk.UpdateDescriptorSets(m_Device, 0, nullptr, copyNum, copies);
}

#include "DescriptorSetVK.hpp"
