// © 2021 NVIDIA Corporation

#include "SharedD3D11.h"
#include "QueryPoolD3D11.h"

using namespace nri;

Result QueryPoolD3D11::Create(const QueryPoolDesc& queryPoolDesc)
{
    m_QueryPool.reserve(queryPoolDesc.capacity);
    m_Type = queryPoolDesc.queryType;

    D3D11_QUERY_DESC queryDesc = {};

    if (m_Type == QueryType::TIMESTAMP)
        queryDesc.Query = D3D11_QUERY_TIMESTAMP;
    else if (m_Type == QueryType::OCCLUSION)
        queryDesc.Query = D3D11_QUERY_OCCLUSION;
    else if (m_Type == QueryType::PIPELINE_STATISTICS)
        queryDesc.Query = D3D11_QUERY_PIPELINE_STATISTICS;
    else
        return Result::UNSUPPORTED;

    for (uint32_t i = 0; i < queryPoolDesc.capacity; i++)
    {
        ID3D11Query* query = nullptr;
        HRESULT hr = m_Device.GetDevice()->CreateQuery(&queryDesc, &query);
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateQuery()");

        m_QueryPool.push_back(query);
    }

    return Result::SUCCESS;
}

void QueryPoolD3D11::BeginQuery(const VersionedContext& deferredContext, uint32_t offset)
{
    ID3D11Query* query = m_QueryPool[offset];

    deferredContext->Begin(query);
}

void QueryPoolD3D11::EndQuery(const VersionedContext& deferredContext, uint32_t offset)
{
    ID3D11Query* query = m_QueryPool[offset];

    deferredContext->End(query);
}

void QueryPoolD3D11::GetData(uint8_t* dstMemory, uint32_t offset, uint32_t num) const
{
    uint32_t dataSize = GetQuerySize();

    num += offset;

    for (uint32_t i = offset; i < num; i++)
    {
        ID3D11Query* query = m_QueryPool[i];
        m_Device.GetImmediateContext()->GetData(query, dstMemory, dataSize, 0);

        dstMemory += dataSize;
    }
}

//================================================================================================================
// NRI
//================================================================================================================

inline void QueryPoolD3D11::SetDebugName(const char* name)
{
    for(ComPtr<ID3D11Query>& query : m_QueryPool)
        SET_D3D_DEBUG_OBJECT_NAME(query, name);
}

inline uint32_t QueryPoolD3D11::GetQuerySize() const
{
    if (m_Type == QueryType::PIPELINE_STATISTICS)
        return sizeof(D3D11_QUERY_DATA_PIPELINE_STATISTICS);

    return sizeof(uint64_t);
}

#include "QueryPoolD3D11.hpp"
