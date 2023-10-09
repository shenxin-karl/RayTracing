#include "FrameResource.h"
#include "CommandListPool.h"
#include "Context.h"
#include "Device.h"

#include <ranges>
#include <unordered_set>

namespace dx {

FrameResource::FrameResource() : _pDevice(nullptr), _fenceValue(0), _graphicsContextIndex(0) {
#if ENABLE_D3D_COMPUTE_QUEUE
    _computeContextIndex = 0;
#endif
}

FrameResource::~FrameResource() {
}

void FrameResource::OnCreate(Device *pDevice, uint32_t numGraphicsCmdListPreFrame, uint32_t numComputeCmdListPreFrame) {
    _pDevice = pDevice;
    _pGraphicsCmdListPool = std::make_unique<CommandListPool>();
    _pGraphicsCmdListPool->OnCreate(pDevice, numGraphicsCmdListPreFrame + 1, D3D12_COMMAND_LIST_TYPE_DIRECT);

#if ENABLE_D3D_COMPUTE_QUEUE
    _pComputeCmdListPool = std::make_unique<CommandListPool>();
    _pComputeCmdListPool->OnCreate(pDevice, numComputeCmdListPreFrame, D3D12_COMMAND_LIST_TYPE_COMPUTE);
#endif

    _graphicsContextIndex = 0;
    _graphicsContextList.clear();
    for (size_t i = 0; i < numGraphicsCmdListPreFrame; ++i) {
        _graphicsContextList.emplace_back(std::make_shared<GraphicsContext>(pDevice));
    }
#if ENABLE_D3D_COMPUTE_QUEUE
    _computeContextIndex = 0;
    _computeContextList.clear();
    for (size_t i = 0; i < numComputeCmdListPreFrame; ++i) {
        _computeContextList.emplace_back(std::make_shared<ComputeContext>(pDevice));
    }
#endif
}

void FrameResource::OnDestroy() {
    _pGraphicsCmdListPool->OnDestroy();
    _pGraphicsCmdListPool = nullptr;
#if ENABLE_D3D_COMPUTE_QUEUE
    _pComputeCmdListPool->OnDestroy();
    _pComputeCmdListPool = nullptr;
#endif
}

void FrameResource::OnBeginFrame(uint64_t newFenceValue) {
    _fenceValue = newFenceValue;
    _pGraphicsCmdListPool->OnBeginFrame();
    _pGraphicsCmdListPool->OnBeginFrame();
    _graphicsContextIndex = 0;
#if ENABLE_D3D_COMPUTE_QUEUE
    _computeContextIndex = 0;
#endif
}

auto FrameResource::AllocGraphicsContext() -> std::shared_ptr<GraphicsContext> {
    Assert(_graphicsContextIndex < _graphicsContextList.size());
    auto pContext = _graphicsContextList[_graphicsContextIndex++];
    pContext->Reset(_pGraphicsCmdListPool->AllocCommandList());
    return pContext;
}

#if ENABLE_D3D_COMPUTE_QUEUE
auto FrameResource::AllocComputeContext() -> std::shared_ptr<ComputeContext> {
    Assert(_computeContextIndex < _computeContextList.size());
    auto pContext = _computeContextList[_computeContextIndex++];
    pContext->Reset(_pComputeCmdListPool->AllocCommandList());
    return pContext;
}
#endif

void FrameResource::ExecuteContexts(ReadonlyArraySpan<Context *> contexts) {
    using ResourceBarriers = ResourceStateTracker::ResourceBarriers;
    using ResourceState = ResourceStateTracker::ResourceState;

    for (Context *pContext : contexts) {
        pContext->FlushResourceBarriers();
    }

    GlobalResourceState::Lock();

    std::vector<ID3D12CommandList *> graphicsCmdList;
    std::unordered_set<ID3D12Resource *> hashSet;
    ResourceBarriers linkCommandListStateBarriers;

    for (int i = 0; i < static_cast<int>(contexts.Count()); ++i) {
        ResourceStateTracker &resourceStateTracker = contexts[i]->_resourceStateTracker;
        ResourceBarriers &pendingResourceBarriers = resourceStateTracker.GetPendingResourceBarriers();
        for (D3D12_RESOURCE_BARRIER barrier : pendingResourceBarriers) {
            ID3D12Resource *pResource = barrier.Transition.pResource;
            ResourceState *pResourceState = GlobalResourceState::FindResourceState(pResource);
            assert(pResourceState != nullptr);

            // translation all sub resource to after state
            if (barrier.Transition.Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) {
                if (pResourceState->subResourceStateMap.empty() &&
                    pResourceState->state == D3D12_RESOURCE_STATE_COMMON &&
                    ResourceStateTracker::OptimizeResourceBarrierState(barrier.Transition.StateAfter)) {
                    continue;
                }

                hashSet.insert(pResource);
                if (pResourceState->subResourceStateMap.empty()) {
                    barrier.Transition.StateBefore = pResourceState->state;
                    linkCommandListStateBarriers.push_back(barrier);
                    continue;
                }

                for (auto &&[subResource, subResourceState] : pResourceState->subResourceStateMap) {
                    barrier.Transition.Subresource = subResource;
                    barrier.Transition.StateBefore = subResourceState;
                    linkCommandListStateBarriers.push_back(barrier);
                }
                continue;
            }

            hashSet.insert(pResource);
            barrier.Transition.StateBefore = pResourceState->GetSubResourceState(barrier.Transition.Subresource);
            linkCommandListStateBarriers.push_back(barrier);
        }

        pendingResourceBarriers.clear();
        if (linkCommandListStateBarriers.empty()) {
            continue;
        }

        CommandList *pCmdList = nullptr;
        if (i == 0) {
            pCmdList = _pGraphicsCmdListPool->AllocCommandList();
            pCmdList->ResourceBarrier(linkCommandListStateBarriers.size(), linkCommandListStateBarriers.data());
            ThrowIfFailed(pCmdList->Close());
            graphicsCmdList.push_back(pCmdList);
        } else {
            pCmdList = contexts[i - 1]->GetCommandList();
            pCmdList->ResourceBarrier(linkCommandListStateBarriers.size(), linkCommandListStateBarriers.data());
        }

        for (const D3D12_RESOURCE_BARRIER barrier : pendingResourceBarriers) {
            ID3D12Resource *pResource = barrier.Transition.pResource;
            ResourceState *pResourceState = GlobalResourceState::FindResourceState(pResource);
            pResourceState->SetSubResourceState(barrier.Transition.Subresource, barrier.Transition.StateAfter);
        }
    }

    for (Context *pContext : contexts) {
        ThrowIfFailed(pContext->GetCommandList()->Close());
    }

    std::vector<ID3D12CommandList *> computeCmdList;
    for (Context *pContext : contexts) {
        if (pContext->GetContextType() == ContextType::eGraphics) {
            graphicsCmdList.push_back(pContext->GetCommandList());
        } else {
            computeCmdList.push_back(pContext->GetCommandList());
        }
    }

    if (!graphicsCmdList.empty()) {
        _pDevice->GetGraphicsQueue()->ExecuteCommandLists(graphicsCmdList.size(), graphicsCmdList.data());
    }

#if ENABLE_D3D_COMPUTE_QUEUE
    if (!computeCmdList.empty()) {
        _pDevice->GetComputeQueue()->ExecuteCommandLists(computeCmdList.size(), computeCmdList.data());
    }
#endif

    for (ID3D12Resource *pResource : hashSet) {
        ResourceState *pResourceState = GlobalResourceState::FindResourceState(pResource);
        for (auto &subResourceState : pResourceState->subResourceStateMap | std::views::values) {
            if (ResourceStateTracker::OptimizeResourceBarrierState(subResourceState)) {
                subResourceState = D3D12_RESOURCE_STATE_COMMON;
            }
        }
        if (ResourceStateTracker::OptimizeResourceBarrierState(pResourceState->state)) {
            pResourceState->state = D3D12_RESOURCE_STATE_COMMON;
        }
    }

    GlobalResourceState::UnLock();
}

}    // namespace dx
