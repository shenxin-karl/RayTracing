// © 2021 NVIDIA Corporation

#include "SharedVK.h"
#include "QueryPoolVK.h"

using namespace nri;

QueryPoolVK::~QueryPoolVK()
{
    const auto& vk = m_Device.GetDispatchTable();

    if (!m_OwnsNativeObjects)
        return;

    for (uint32_t i = 0; i < GetCountOf(m_Handles); i++)
    {
        if (m_Handles[i] != VK_NULL_HANDLE)
            vk.DestroyQueryPool(m_Device, m_Handles[i], m_Device.GetAllocationCallbacks());
    }
}

Result QueryPoolVK::Create(const QueryPoolDesc& queryPoolDesc)
{
    m_OwnsNativeObjects = true;
    m_Type = GetQueryType(queryPoolDesc.queryType);

    const VkQueryPoolCreateInfo poolInfo = {
        VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
        nullptr,
        (VkQueryPoolCreateFlags)0,
        m_Type,
        queryPoolDesc.capacity,
        GetQueryPipelineStatisticsFlags(queryPoolDesc.pipelineStatsMask)
    };

    const auto& vk = m_Device.GetDispatchTable();

    const uint32_t nodeMask = GetNodeMask(queryPoolDesc.nodeMask);

    for (uint32_t i = 0; i < m_Device.GetPhysicalDeviceGroupSize(); i++)
    {
        if ((1 << i) & nodeMask)
        {
            const VkResult result = vk.CreateQueryPool(m_Device, &poolInfo, m_Device.GetAllocationCallbacks(), &m_Handles[i]);

            RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result),
                "Can't create a query pool: vkCreateQueryPool returned %d.", (int32_t)result);
        }
    }

    m_Stride = GetQuerySize();

    return Result::SUCCESS;
}

Result QueryPoolVK::Create(const QueryPoolVKDesc& queryPoolDesc)
{
    m_OwnsNativeObjects = false;
    m_Type = (VkQueryType)queryPoolDesc.vkQueryType;

    const VkQueryPool handle = (VkQueryPool)queryPoolDesc.vkQueryPool;
    const uint32_t nodeMask = GetNodeMask(queryPoolDesc.nodeMask);

    for (uint32_t i = 0; i < m_Device.GetPhysicalDeviceGroupSize(); i++)
    {
        if ((1 << i) & nodeMask)
            m_Handles[i] = handle;
    }

    m_Stride = GetQuerySize();

    return Result::SUCCESS;
}

//================================================================================================================
// NRI
//================================================================================================================

inline void QueryPoolVK::SetDebugName(const char* name)
{
    std::array<uint64_t, PHYSICAL_DEVICE_GROUP_MAX_SIZE> handles;
    for (size_t i = 0; i < handles.size(); i++)
        handles[i] = (uint64_t)m_Handles[i];

    m_Device.SetDebugNameToDeviceGroupObject(VK_OBJECT_TYPE_QUERY_POOL, handles.data(), name);
}

inline uint32_t QueryPoolVK::GetQuerySize() const
{
    const uint32_t highestBitInPipelineStatsBits = 11;

    switch (m_Type)
    {
    case VK_QUERY_TYPE_TIMESTAMP:
        return sizeof(uint64_t);
    case VK_QUERY_TYPE_OCCLUSION:
        return sizeof(uint64_t);
    case VK_QUERY_TYPE_PIPELINE_STATISTICS:
        return highestBitInPipelineStatsBits * sizeof(uint64_t);
    default:
        CHECK(&m_Device, false, "unexpected query type in GetQuerySize: %u", (uint32_t)m_Type);
        return 0;
    }
}

#include "QueryPoolVK.hpp"