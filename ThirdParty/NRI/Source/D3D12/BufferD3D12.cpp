// © 2021 NVIDIA Corporation

#include "SharedD3D12.h"
#include "BufferD3D12.h"
#include "MemoryD3D12.h"

using namespace nri;

static D3D12_RESOURCE_DESC GetResourceDesc(const BufferDesc& bufferDesc)
{
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT; // 64KB
    desc.Width = bufferDesc.size;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = GetBufferFlags(bufferDesc.usageMask);

    return desc;
}

Result BufferD3D12::Create(const BufferDesc& bufferDesc)
{
    m_Desc = bufferDesc;

    return Result::SUCCESS;
}

Result BufferD3D12::Create(const BufferD3D12Desc& bufferDesc)
{
    if (!GetBufferDesc(bufferDesc, m_Desc))
        return Result::INVALID_ARGUMENT;

    m_Buffer = bufferDesc.d3d12Resource;

    return Result::SUCCESS;
}

Result BufferD3D12::BindMemory(const MemoryD3D12* memory, uint64_t offset, bool isAccelerationStructureBuffer)
{
    const D3D12_HEAP_DESC& heapDesc = memory->GetHeapDesc();
    D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;
    D3D12_RESOURCE_DESC resourceDesc = GetResourceDesc(m_Desc);

    if (heapDesc.Properties.Type == D3D12_HEAP_TYPE_UPLOAD)
        initialState |= D3D12_RESOURCE_STATE_GENERIC_READ;
    else if (heapDesc.Properties.Type == D3D12_HEAP_TYPE_READBACK)
        initialState |= D3D12_RESOURCE_STATE_COPY_DEST;

    if (isAccelerationStructureBuffer)
        initialState |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

    if (memory->RequiresDedicatedAllocation())
    {
        HRESULT hr = ((ID3D12Device*)m_Device)->CreateCommittedResource(
            &heapDesc.Properties,
            D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
            &resourceDesc,
            initialState,
            nullptr,
            IID_PPV_ARGS(&m_Buffer)
        );
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreateCommittedResource()");
    }
    else
    {
        HRESULT hr = ((ID3D12Device*)m_Device)->CreatePlacedResource(
            *memory,
            offset,
            &resourceDesc,
            initialState,
            nullptr,
            IID_PPV_ARGS(&m_Buffer)
        );
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreatePlacedResource()");
    }

    return Result::SUCCESS;
}

//================================================================================================================
// NRI
//================================================================================================================

inline void BufferD3D12::GetMemoryInfo(MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const
{
    D3D12_RESOURCE_DESC resourceDesc = GetResourceDesc(m_Desc);
    m_Device.GetMemoryInfo(memoryLocation, resourceDesc, memoryDesc);
}

inline void* BufferD3D12::Map(uint64_t offset, uint64_t size)
{
    uint8_t* data = nullptr;

    if (size == WHOLE_SIZE)
        size =  m_Desc.size;

    D3D12_RANGE range = {(SIZE_T)offset, (SIZE_T)(offset + size)};
    HRESULT hr = m_Buffer->Map(0, &range, (void**)&data);
    if (FAILED(hr))
        REPORT_ERROR(&m_Device, "ID3D12Resource::Map() failed, result = 0x%08X!", hr);

    return data + offset;
}

inline void BufferD3D12::Unmap()
{
    m_Buffer->Unmap(0, nullptr);
}

#include "BufferD3D12.hpp"
