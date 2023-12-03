#include "UploadHeap.h"
#include "Device.h"
#include "ResourceStateTracker.h"

namespace dx {

UploadHeap::UploadHeap() {
}

UploadHeap::~UploadHeap() {
}

void UploadHeap::OnCreate(Device *pDevice, size_t size) {
    _pDevice = pDevice;
    ID3D12Device *device = pDevice->GetNativeDevice();

    device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_pCommandAllocator));
    device->CreateCommandList(0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        _pCommandAllocator.Get(),
        nullptr,
        IID_PPV_ARGS(&_pCommandList));

    _pCommandAllocator->SetName(L"UploadHeap::CommandListAllocator");
    _pCommandList->SetName(L"UploadHeap::CommandList");

    _pUploadFinishedFence = std::make_unique<Fence>();
    _pUploadFinishedFence->OnCreate(_pDevice, "UploadHeap::UploadFinishedFence");

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
    if (_pUploadFinishedFence != nullptr) {
	    _pUploadFinishedFence->OnDestroy();
        _pUploadFinishedFence = nullptr;
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
    GlobalResourceState::Lock();

    using ResourceStateMap = ResourceStateTracker::ResourceStateMap;
    using ResourceBarriers = ResourceStateTracker::ResourceBarriers;
    ResourceStateMap resourceStateMap;

    auto DoResourceBarrier = [&](const ResourceBarriers &barriers) {
        ResourceBarriers tempBarriers;
        for (D3D12_RESOURCE_BARRIER barrier : barriers) {
            ID3D12Resource *pResource = barrier.Transition.pResource;

            ResourceStateTracker::ResourceState *pResourceState = nullptr;
            if (auto it = resourceStateMap.find(pResource); it != resourceStateMap.end()) {
                pResourceState = &it->second;
            }
            if (pResourceState == nullptr) {
                pResourceState = GlobalResourceState::FindResourceState(barrier.Transition.pResource);
            }
            Assert(pResourceState != nullptr);

            // translation all sub resource to after state
            if (barrier.Transition.Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) {
                if (pResourceState->subResourceStateMap.empty() &&
                    pResourceState->state == D3D12_RESOURCE_STATE_COMMON &&
                    ResourceStateTracker::OptimizeResourceBarrierState(barrier.Transition.StateAfter)) {
                    resourceStateMap[pResource].SetSubResourceState(barrier.Transition.Subresource,
                        barrier.Transition.StateAfter);
                    continue;
                }

                if (pResourceState->subResourceStateMap.empty()) {
                    barrier.Transition.StateBefore = pResourceState->state;
                    tempBarriers.push_back(barrier);
                    resourceStateMap[pResource].SetSubResourceState(barrier.Transition.Subresource,
                        barrier.Transition.StateAfter);
                    continue;
                }

                for (auto &&[subResource, subResourceState] : pResourceState->subResourceStateMap) {
                    barrier.Transition.Subresource = subResource;
                    barrier.Transition.StateBefore = subResourceState;
                    resourceStateMap[pResource].SetSubResourceState(barrier.Transition.Subresource,
                        barrier.Transition.StateAfter);
                    tempBarriers.push_back(barrier);
                }
                continue;
            }
            barrier.Transition.StateBefore = pResourceState->GetSubResourceState(barrier.Transition.Subresource);
            resourceStateMap[pResource].SetSubResourceState(barrier.Transition.Subresource,
                barrier.Transition.StateAfter);
            tempBarriers.push_back(barrier);
        }

        if (tempBarriers.size()) {
            _pCommandList->ResourceBarrier(tempBarriers.size(), tempBarriers.data());
        }
        tempBarriers.clear();
    };

    if (!_preUploadBarriers.empty()) {
        DoResourceBarrier(_preUploadBarriers);
    }

    for (const TextureCopy &c : _textureCopies) {
        _pCommandList->CopyTextureRegion(&c.dst, 0, 0, 0, &c.src, nullptr);
    }

    for (const BufferCopy &c : _bufferCopies) {
        _pCommandList->CopyBufferRegion(c.pDestBuffer, c.dstOffset, GetResource(), c.srcOffset, c.size);
    }

    if (!_postUploadBarriers.empty()) {
        DoResourceBarrier(_postUploadBarriers);
    }

    _textureCopies.clear();
    _bufferCopies.clear();
    _preUploadBarriers.clear();
    _postUploadBarriers.clear();

    ThrowIfFailed(_pCommandList->Close());
    ID3D12CommandList *cmdList[] = {_pCommandList.Get()};
    _pDevice->GetCopyQueue()->ExecuteCommandLists(1, cmdList);
    _pUploadFinishedFence->IssueFence(_pDevice->GetCopyQueue());
    CpuWaitForUploadFinished();

    // update resource state
    for (auto &&[pResource, resourceState] : resourceStateMap) {
        auto pResourceState = GlobalResourceState::FindResourceState(pResource);
        if (resourceState.subResourceStateMap.empty()) {
            D3D12_RESOURCE_STATES state = resourceState.state;
            if (ResourceStateTracker::OptimizeResourceBarrierState(resourceState.state)) {
                state = D3D12_RESOURCE_STATE_COMMON;
            }
            pResourceState->SetSubResourceState(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, state);
            continue;
        }

        for (auto &&[subResource, subResourceState] : resourceState.subResourceStateMap) {
            if (ResourceStateTracker::OptimizeResourceBarrierState(resourceState.state)) {
                subResourceState = D3D12_RESOURCE_STATE_COMMON;
            }
            pResourceState->SetSubResourceState(subResource, subResourceState);
        }
    }
    GlobalResourceState::UnLock();
}

void UploadHeap::CpuWaitForUploadFinished() {
    if (!_bufferCopies.empty() || !_textureCopies.empty() || !_preUploadBarriers.empty() || !_postUploadBarriers.empty()) {
	    _pUploadFinishedFence->CpuWaitForFence();
	    ThrowIfFailed(_pCommandAllocator->Reset());
	    ThrowIfFailed(_pCommandList->Reset(_pCommandAllocator.Get(), nullptr));
	    _pDataCur = _pDataBegin;
    }

}

void UploadHeap::AddPreUploadTranslation(ID3D12Resource *pResource,
                                         D3D12_RESOURCE_STATES stateAfter,
                                         UINT subResource,
                                         D3D12_RESOURCE_BARRIER_FLAGS flags) {

    D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(pResource,
        D3D12_RESOURCE_STATE_COMMON,
        stateAfter,
        subResource,
        flags);
    _preUploadBarriers.push_back(barrier);
}

void UploadHeap::AddPostUploadTranslation(ID3D12Resource *pResource,
    D3D12_RESOURCE_STATES stateAfter,
    UINT subResource,
    D3D12_RESOURCE_BARRIER_FLAGS flags) {

    D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(pResource,
        D3D12_RESOURCE_STATE_COMMON,
        stateAfter,
        subResource,
        flags);
    _postUploadBarriers.push_back(barrier);
}

}    // namespace dx