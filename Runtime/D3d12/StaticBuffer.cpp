#include "StaticBuffer.h"

#include "Device.h"
#include "ResourceStateTracker.h"
#include "UploadHeap.h"
#include "Foundation/StringUtil.h"

namespace dx {

StaticBuffer::StaticBuffer(std::source_location sl): _pDevice(nullptr), _pAllocation(nullptr), _bufferDesc() {
	_name = fmt::format("Buffer {}:{}", sl.file_name(), sl.line());
}

StaticBuffer::~StaticBuffer() {
    OnDestroy();
}

void StaticBuffer::OnCreate(Device *pDevice, size_t totalMemSize, D3D12_RESOURCE_FLAGS flags, UINT64 alignment) {
	OnCreate(pDevice, CD3DX12_RESOURCE_DESC::Buffer(totalMemSize, flags, alignment));
}

void StaticBuffer::OnCreate(Device *pDevice, const D3D12_RESOURCE_DESC &desc) {
	_pDevice = pDevice;
	_bufferDesc = desc;

	D3D12MA::Allocator *pAllocator = pDevice->GetAllocator();
    D3D12MA::ALLOCATION_DESC allocationDesc = {};
    allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
    ThrowIfFailed(pAllocator->CreateResource(
        &allocationDesc,
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

void StaticBuffer::OnDestroy() {
    if (_pAllocation == nullptr) {
	    return;
    }

    _pAllocation->Release();
    _pAllocation = nullptr;
    _pDevice = nullptr;
}

StaticBufferUploadHeap::StaticBufferUploadHeap(StaticBuffer &staticBuffer, UploadHeap &uploadHeap) {
    _pStaticBuffer = &staticBuffer;
    _pUploadHeap = &uploadHeap;
}

StaticBufferUploadHeap::~StaticBufferUploadHeap() {
}

auto StaticBufferUploadHeap::AllocVertexBuffer(size_t numOfVertices, size_t strideInBytes,
	const void *pData) -> std::optional<D3D12_VERTEX_BUFFER_VIEW> {

    size_t bufferSize = numOfVertices * strideInBytes;
    size_t alignmentOffset = AlignUp(_offset, strideInBytes);
    size_t newOffset = alignmentOffset + bufferSize;
    if (newOffset > _pStaticBuffer->GetBufferSize()) {
	    return std::nullopt;
    }

    uint8_t *pBuffer = _pUploadHeap->AllocBuffer(bufferSize);
    if (pBuffer == nullptr) {
	    return std::nullopt;
    }

    std::memcpy(pBuffer, pData, bufferSize);
    _offset = newOffset;

    UploadHeap::BufferCopy bufferCopy;
    bufferCopy.size = bufferSize;
    bufferCopy.offset = _pUploadHeap->CalcBufferOffset(pBuffer);
    bufferCopy.pDestBuffer = _pStaticBuffer->GetResource();
    _pUploadHeap->AddBufferCopy(bufferCopy);

    D3D12_VERTEX_BUFFER_VIEW view;
    view.BufferLocation = _pStaticBuffer->GetResource()->GetGPUVirtualAddress() + alignmentOffset;
    view.SizeInBytes = bufferSize;
    view.StrideInBytes = strideInBytes;
    return std::make_optional(view);
}

auto StaticBufferUploadHeap::AllocIndexBuffer(size_t numOfVertices, size_t strideInBytes,
	const void *pData) -> std::optional<D3D12_INDEX_BUFFER_VIEW> {
}

auto StaticBufferUploadHeap::AllocConstantBuffer(size_t totalMemory,
	const void *pData) -> std::optional<D3D12_CONSTANT_BUFFER_VIEW_DESC> {
}

auto StaticBufferUploadHeap::AllocStructuredBuffer(size_t numOfVertices, size_t strideInBytes,
	const void *pData) -> std::optional<SRV> {
}

void StaticBufferUploadHeap::DoUpload() {
    _pUploadHeap->DoUpload();
}

}    // namespace dx
