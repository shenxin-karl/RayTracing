// © 2021 NVIDIA Corporation

#include "SharedD3D11.h"
#include "DescriptorSetD3D11.h"
#include "PipelineLayoutD3D11.h"

using namespace nri;

#define BASE_CONSTANT_BUFFER 0
#define BASE_RANGE m_DynamicConstantBuffersNum

uint32_t DescriptorSetD3D11::Initialize(const PipelineLayoutD3D11& pipelineLayout, uint32_t setIndexInPipelineLayout, const DescriptorD3D11** descriptors)
{
    const BindingSet& bindingSet = pipelineLayout.GetBindingSet(setIndexInPipelineLayout);

    // Reset head, since this object can be reused via DescriptorPool::Reset() and DescriptorPool::AllocateDescriptorSets()
    m_Ranges.clear();

    // Constant buffers first
    for (uint32_t i = bindingSet.rangeStart; i < bindingSet.rangeEnd; i++)
    {
        const BindingRange& bindingRange = pipelineLayout.GetBindingRange(i);
        if (bindingRange.descriptorType == DescriptorTypeDX11::DYNAMIC_CONSTANT)
        {
            OffsetNum offsetNum = {};
            offsetNum.descriptorOffset = bindingRange.descriptorOffset;
            offsetNum.descriptorNum = bindingRange.descriptorNum;

            m_Ranges.push_back(offsetNum);
        }
    }

    m_DynamicConstantBuffersNum = (uint32_t)m_Ranges.size();

    // Then others
    for (uint32_t i = bindingSet.rangeStart; i < bindingSet.rangeEnd; i++)
    {
        const BindingRange& bindingRange = pipelineLayout.GetBindingRange(i);
        if (bindingRange.descriptorType != DescriptorTypeDX11::DYNAMIC_CONSTANT)
        {
            OffsetNum offsetNum = {};
            offsetNum.descriptorOffset = bindingRange.descriptorOffset;
            offsetNum.descriptorNum = bindingRange.descriptorNum;

            m_Ranges.push_back(offsetNum);
        }
    }

    m_Descriptors = descriptors;

    return bindingSet.descriptorNum;
}

//================================================================================================================
// NRI
//================================================================================================================

inline void DescriptorSetD3D11::UpdateDescriptorRanges(uint32_t rangeOffset, uint32_t rangeNum, const DescriptorRangeUpdateDesc* rangeUpdateDescs)
{
    for (uint32_t i = 0; i < rangeNum; i++)
    {
        const DescriptorRangeUpdateDesc& range = rangeUpdateDescs[i];

        uint32_t descriptorOffset = m_Ranges[BASE_RANGE + rangeOffset + i].descriptorOffset;
        descriptorOffset += range.offsetInRange;

        const DescriptorD3D11** dstDescriptors = m_Descriptors + descriptorOffset;
        const DescriptorD3D11** srcDescriptors = (const DescriptorD3D11**)range.descriptors;

        memcpy(dstDescriptors, srcDescriptors, range.descriptorNum * sizeof(DescriptorD3D11*));
    }
}

inline void DescriptorSetD3D11::UpdateDynamicConstantBuffers(uint32_t baseBuffer, uint32_t bufferNum, const Descriptor* const* descriptors)
{
    const DescriptorD3D11** srcDescriptors = (const DescriptorD3D11**)descriptors;

    for (uint32_t i = 0; i < bufferNum; i++)
    {
        uint32_t descriptorOffset = m_Ranges[BASE_CONSTANT_BUFFER + baseBuffer + i].descriptorOffset;
        m_Descriptors[descriptorOffset] = srcDescriptors[i];
    }
}

inline void DescriptorSetD3D11::Copy(const DescriptorSetCopyDesc& descriptorSetCopyDesc)
{
    DescriptorSetD3D11& srcSet = (DescriptorSetD3D11&)descriptorSetCopyDesc.srcDescriptorSet;

    for (uint32_t i = 0; i < descriptorSetCopyDesc.rangeNum; i++)
    {
        const OffsetNum& dst = m_Ranges[BASE_RANGE + descriptorSetCopyDesc.baseDstRange + i];
        const DescriptorD3D11** dstDescriptors = m_Descriptors + dst.descriptorOffset;

        const OffsetNum& src = srcSet.m_Ranges[BASE_RANGE + descriptorSetCopyDesc.baseSrcRange + i];
        const DescriptorD3D11** srcDescriptors = srcSet.m_Descriptors + src.descriptorOffset;

        memcpy(dstDescriptors, srcDescriptors, dst.descriptorNum * sizeof(DescriptorD3D11*));
    }

    for (uint32_t i = 0; i < descriptorSetCopyDesc.dynamicConstantBufferNum; i++)
    {
        const OffsetNum& dst = m_Ranges[BASE_CONSTANT_BUFFER + descriptorSetCopyDesc.baseDstDynamicConstantBuffer + i];
        const OffsetNum& src = srcSet.m_Ranges[BASE_CONSTANT_BUFFER + descriptorSetCopyDesc.baseSrcDynamicConstantBuffer + i];

        m_Descriptors[dst.descriptorOffset] = srcSet.m_Descriptors[src.descriptorOffset];
    }
}

#include "DescriptorSetD3D11.hpp"
