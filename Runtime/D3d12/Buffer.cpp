#include "Buffer.h"
#include "DescriptorManager.hpp"
#include "Device.h"
#include "ResourceStateTracker.h"
#include "UploadHeap.h"
#include "Foundation/StringUtil.h"

namespace dx {

Buffer::Buffer(Device *pDevice, D3D12_HEAP_TYPE heapType, const D3D12_RESOURCE_DESC &desc) {
    _pDevice = pDevice;
    _bufferDesc = desc;
    _heapType = heapType;

    D3D12MA::Allocator *pAllocator = pDevice->GetAllocator();
    D3D12MA::ALLOCATION_DESC allocationDesc = {};
    allocationDesc.HeapType = heapType;
    ThrowIfFailed(pAllocator->CreateResource(&allocationDesc,
        &_bufferDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        &_pAllocation,
        IID_NULL,
        nullptr));

    ID3D12Resource *pResource = _pAllocation->GetResource();
    pResource->SetName(nstd::to_wstring(_name).c_str());
    GlobalResourceState::SetResourceState(pResource, D3D12_RESOURCE_STATE_COMMON);
}

Buffer::~Buffer() {
    if (_pAllocation == nullptr) {
        return;
    }

    GlobalResourceState::RemoveResourceState(_pAllocation->GetResource());
    _pAllocation->Release();
    _pAllocation = nullptr;
    _pDevice = nullptr;
}

StaticBufferUploadHeap::StaticBufferUploadHeap(UploadHeap *pUploadHeap,
    Buffer *pStaticBuffer,
    size_t staticBufferOffset) {

    _pStaticBuffer = pStaticBuffer;
    Exception::CondThrow(pStaticBuffer->IsStaticBuffer(), "StaticBufferUploadHeap::pStaticBuffer most static buffer!");
    _dstOffset = staticBufferOffset;
    _pUploadHeap = pUploadHeap;
    _pUploadHeap->AddPreUploadTranslation(_pStaticBuffer->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST);
}

StaticBufferUploadHeap::~StaticBufferUploadHeap() {
}

auto StaticBufferUploadHeap::AllocVertexBuffer(size_t numOfVertices, size_t strideInBytes, const void *pData)
    -> std::optional<D3D12_VERTEX_BUFFER_VIEW> {

    std::optional<BufferInResourceInfo> pInfo = CopyToUploadBuffer(numOfVertices, strideInBytes, pData);
    if (!pInfo.has_value()) {
        return std::nullopt;
    }

    _vertexBuffer = true;
    D3D12_VERTEX_BUFFER_VIEW view;
    view.BufferLocation = pInfo->virtualAddress;
    view.SizeInBytes = pInfo->bufferSize;
    view.StrideInBytes = strideInBytes;
    return std::make_optional(view);
}

auto StaticBufferUploadHeap::AllocIndexBuffer(size_t numOfVertices, size_t strideInBytes, const void *pData)
    -> std::optional<D3D12_INDEX_BUFFER_VIEW> {

    std::optional<BufferInResourceInfo> pInfo = CopyToUploadBuffer(numOfVertices, strideInBytes, pData);
    if (!pInfo.has_value()) {
        return std::nullopt;
    }

    _indexBuffer = true;
    D3D12_INDEX_BUFFER_VIEW view;
    view.BufferLocation = pInfo->virtualAddress;
    view.SizeInBytes = pInfo->bufferSize;
    if (strideInBytes == 2) {
        view.Format = DXGI_FORMAT_R16_UINT;
    } else if (strideInBytes == 4) {
        view.Format = DXGI_FORMAT_R32_UINT;
    } else {
        Assert("Invalid Index Format");
    }
    return std::make_optional(view);
}

auto StaticBufferUploadHeap::AllocConstantBuffer(size_t totalMemory, const void *pData)
    -> std::optional<D3D12_CONSTANT_BUFFER_VIEW_DESC> {

    size_t alignedTotalMemory = AlignUp(totalMemory, 256);
    std::optional<BufferInResourceInfo> pInfo = CopyToUploadBuffer(1, alignedTotalMemory, pData, totalMemory);
    if (!pInfo.has_value()) {
        return std::nullopt;
    }

    _constantBuffer = true;
    D3D12_CONSTANT_BUFFER_VIEW_DESC view;
    view.BufferLocation = pInfo->virtualAddress;
    view.SizeInBytes = alignedTotalMemory;
    return std::make_optional(view);
}

auto StaticBufferUploadHeap::AllocStructuredBuffer(size_t numOfVertices, size_t strideInBytes, const void *pData)
    -> std::optional<SRV> {

    std::optional<BufferInResourceInfo> pInfo = CopyToUploadBuffer(numOfVertices, strideInBytes, pData);
    if (!pInfo.has_value()) {
        return std::nullopt;
    }

    _structuredBuffer = true;
    SRV srv = _pStaticBuffer->GetDevice()->AllocDescriptor<SRV>(1);
    ID3D12Device *device = _pStaticBuffer->GetDevice()->GetNativeDevice();

    D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
    desc.Format = _pStaticBuffer->GetDesc().Format;
    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    desc.Buffer.FirstElement = _srcOffset / strideInBytes;
    desc.Buffer.NumElements = static_cast<UINT>(numOfVertices);
    desc.Buffer.StructureByteStride = static_cast<UINT>(strideInBytes);
    desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    device->CreateShaderResourceView(_pStaticBuffer->GetResource(), &desc, srv.GetCpuHandle());
    return std::make_optional(srv);
}

void StaticBufferUploadHeap::CommitUploadCommand() {
    D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
    //if (_vertexBuffer || _constantBuffer) {
    //    state |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    //}
    //if (_indexBuffer) {
    //    state |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
    //}
    //if (_structuredBuffer) {
    //    state |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    //}
    _pUploadHeap->AddPostUploadTranslation(_pStaticBuffer->GetResource(), state);
}

auto StaticBufferUploadHeap::CopyToUploadBuffer(size_t numOfVertices,
    size_t strideInBytes,
    const void *pData,
    size_t dataSize) -> std::optional<BufferInResourceInfo> {

    size_t originOffset = _srcOffset;
    size_t bufferSize = numOfVertices * strideInBytes;
    size_t newOffset = _srcOffset + bufferSize;
    if (newOffset > _pStaticBuffer->GetDesc().Width) {
        return std::nullopt;
    }

    uint8_t *pBuffer = _pUploadHeap->AllocBuffer(bufferSize);
    if (pBuffer == nullptr) {
        return std::nullopt;
    }

    std::memcpy(pBuffer, pData, dataSize);
    size_t originDstOffset = _dstOffset;
    _srcOffset = newOffset;
    _dstOffset += bufferSize;

    UploadHeap::BufferCopy bufferCopy;
    bufferCopy.size = bufferSize;
    bufferCopy.srcOffset = _pUploadHeap->CalcBufferOffset(pBuffer);
    bufferCopy.dstOffset = originDstOffset;
    bufferCopy.pDestBuffer = _pStaticBuffer->GetResource();
    _pUploadHeap->AddBufferCopy(bufferCopy);

    BufferInResourceInfo info;
    info.bufferSize = bufferSize;
    info.virtualAddress = _pStaticBuffer->GetResource()->GetGPUVirtualAddress() + originOffset;
    return std::make_optional(info);
}

auto StaticBufferUploadHeap::CopyToUploadBuffer(size_t numOfVertices, size_t strideInBytes, const void *pData)
    -> std::optional<BufferInResourceInfo> {
    return CopyToUploadBuffer(numOfVertices, strideInBytes, pData, numOfVertices * strideInBytes);
}

}    // namespace dx
