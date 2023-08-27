#include "UploadHeap.h"
#include "Device.h"

namespace dx {

UploadHeap::UploadHeap() {
}

UploadHeap::~UploadHeap() {
}

void UploadHeap::OnCreate(Device *pDevice, size_t size) {
    _pDevice = pDevice;
    ID3D12Device *device = pDevice->GetDevice();

    device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&_pCommandAllocator));
    device->CreateCommandList(0,
        D3D12_COMMAND_LIST_TYPE_COPY,
        _pCommandAllocator.Get(),
        nullptr,
        IID_PPV_ARGS(&_pCommandList));

    _pCommandAllocator->SetName(L"UploadHeap::CommandListAllocator");
    _pCommandList->SetName(L"UploadHeap::CommandList");

    D3D12MA::Allocator *pAllocator = pDevice->GetAllocator();
    D3D12MA::ALLOCATION_DESC allocationDesc = {};
    allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
    ThrowIfFailed(pAllocator->CreateResource(&allocationDesc,
        RVPtr(CD3DX12_RESOURCE_DESC::Buffer(size)),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        &_pBufferAllocation,
        IID_NULL,
        nullptr));

    ID3D12Resource *pResource = _pBufferAllocation->GetResource();
    pResource->SetName(L"UploadHeap");

    ThrowIfFailed(pResource->Map(0, nullptr, reinterpret_cast<void **>(&_pDataBegin)));
    _pDataCur = _pDataBegin;
    _pDataEnd = _pDataBegin + size;
}

void UploadHeap::OnDestroy() {
    if (_pDataCur != _pDataBegin) {
        Exception::Throw("did not complete the upload, but destroyed");
    }

    if (_pBufferAllocation) {
        _pBufferAllocation->Release();
    }
    _pDataBegin = nullptr;
    _pDataCur = nullptr;
    _pDataEnd = nullptr;
}

auto UploadHeap::AllocBuffer(size_t size, size_t align) -> uint8_t * {
    bool doUploaded = false;
    while (true) {
        uint8_t *ptr = reinterpret_cast<uint8_t *>(AlignUp<ptrdiff_t>(reinterpret_cast<ptrdiff_t>(_pDataCur), align));
        if (ptr >= _pDataEnd || ptr + size >= _pDataEnd) {
            if (!doUploaded) {
                DoUpload();
                doUploaded = true;
                continue;
            }
            return nullptr;
        }
        _pDataCur += size;
        return ptr;
    }
}

auto UploadHeap::GetAllocatableSize(size_t align) const -> size_t {
    uint8_t *ptr = reinterpret_cast<uint8_t *>(AlignUp<ptrdiff_t>(reinterpret_cast<ptrdiff_t>(_pDataCur), align));
    if (ptr >= _pDataEnd) {
        return 0;
    }
    return _pDataEnd - ptr;
}

void UploadHeap::DoUpload() {
    if (!_preUploadBarriers.empty()) {
        _pCommandList->ResourceBarrier(_preUploadBarriers.size(), _preUploadBarriers.data());
    }

    for (const TextureCopy &c : _textureCopies) {
        _pCommandList->CopyTextureRegion(&c.dst, 0, 0, 0, &c.src, nullptr);
    }

    for (const BufferCopy &c : _bufferCopies) {
        _pCommandList->CopyBufferRegion(c.pDestBuffer, 0, GetResource(), c.offset, c.size);
    }

    if (!_postUploadBarriers.empty()) {
        _pCommandList->ResourceBarrier(_postUploadBarriers.size(), _postUploadBarriers.data());
    }

    _textureCopies.clear();
    _bufferCopies.clear();
    _preUploadBarriers.clear();
    _postUploadBarriers.clear();

    ThrowIfFailed(_pCommandList->Close());
    ID3D12CommandList *cmdList[] = {_pCommandList.Get()};
    _pDevice->GetCopyQueue()->ExecuteCommandLists(1, cmdList);
    _pDevice->WaitForGPUFlush(D3D12_COMMAND_LIST_TYPE_COPY);
    ThrowIfFailed(_pCommandAllocator->Reset());
    ThrowIfFailed(_pCommandList->Reset(_pCommandAllocator.Get(), nullptr));

    _pDataCur = _pDataBegin;
}

}    // namespace dx