// © 2021 NVIDIA Corporation

#include "SharedD3D11.h"
#include "DescriptorSetD3D11.h"
#include "DescriptorPoolD3D11.h"

using namespace nri;

Result DescriptorPoolD3D11::Create(const DescriptorPoolDesc& descriptorPoolDesc)
{
    uint32_t descriptorNum = descriptorPoolDesc.samplerMaxNum;
    descriptorNum += descriptorPoolDesc.samplerMaxNum;
    descriptorNum += descriptorPoolDesc.constantBufferMaxNum;
    descriptorNum += descriptorPoolDesc.dynamicConstantBufferMaxNum;
    descriptorNum += descriptorPoolDesc.textureMaxNum;
    descriptorNum += descriptorPoolDesc.storageTextureMaxNum;
    descriptorNum += descriptorPoolDesc.bufferMaxNum;
    descriptorNum += descriptorPoolDesc.storageBufferMaxNum;
    descriptorNum += descriptorPoolDesc.structuredBufferMaxNum;
    descriptorNum += descriptorPoolDesc.storageStructuredBufferMaxNum;

    m_DescriptorPool.resize(descriptorNum, nullptr);
    m_DescriptorSets.resize(descriptorPoolDesc.descriptorSetMaxNum, DescriptorSetD3D11(m_Device));

    return Result::SUCCESS;
}

//================================================================================================================
// NRI
//================================================================================================================

inline Result DescriptorPoolD3D11::AllocateDescriptorSets(const PipelineLayout& pipelineLayout, uint32_t setIndexInPipelineLayout, DescriptorSet** descriptorSets,
    uint32_t instanceNum, uint32_t nodeMask, uint32_t variableDescriptorNum)
{
    MaybeUnused(nodeMask);
    MaybeUnused(variableDescriptorNum); // TODO: report error if non-0?

    const PipelineLayoutD3D11& pipelineLayoutD3D11 = (PipelineLayoutD3D11&)pipelineLayout;

    for (uint32_t i = 0; i < instanceNum; i++)
    {
        const DescriptorD3D11** descriptors = m_DescriptorPool.data() + m_DescriptorPoolOffset;
        DescriptorSetD3D11* descriptorSet = &m_DescriptorSets[m_DescriptorSetIndex++];
        uint32_t descriptorNum = descriptorSet->Initialize(pipelineLayoutD3D11, setIndexInPipelineLayout, descriptors);
        descriptorSets[i] = (DescriptorSet*)descriptorSet;

        m_DescriptorPoolOffset += descriptorNum;
    }

    return Result::SUCCESS;
}

#include "DescriptorPoolD3D11.hpp"
