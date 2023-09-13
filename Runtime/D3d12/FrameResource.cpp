#include "FrameResource.h"

#include <ranges>
#include <unordered_set>

#include "CommandListPool.h"
#include "Context.h"
#include "Device.h"

namespace dx {

FrameResource::FrameResource() : _pDevice(nullptr), _fenceValue(0) {
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
}

auto FrameResource::AllocGraphicsContext() -> std::shared_ptr<GraphicsContext> {
    return std::make_shared<GraphicsContext>(_pGraphicsCmdListPool->AllocCommandList());
}

#if ENABLE_D3D_COMPUTE_QUEUE
auto FrameResource::AllocComputeContext() -> std::shared_ptr<ComputeContext> {
    return std::make_shared<ComputeContext>(_pComputeCmdListPool->AllocCommandList());
}
#endif

// 下面这些状态,能够被 D3D12_RESOURCE_STATE_COMMON 隐式转换, 在 ExecuteCommandLists 后, 也能够自动转化为 D3D12_RESOURCE_STATE_COMMON
static bool OptimizeResourceBarrierState(D3D12_RESOURCE_STATES state) {
    switch (state) {
    case D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:
    case D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:
    case D3D12_RESOURCE_STATE_COPY_DEST:
    case D3D12_RESOURCE_STATE_COPY_SOURCE:
    case D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER:
    case D3D12_RESOURCE_STATE_INDEX_BUFFER:
    case D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT:
    case D3D12_RESOURCE_STATE_GENERIC_READ:
        return true;
    default:
        return false;
    }
}

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
                    OptimizeResourceBarrierState(barrier.Transition.StateAfter)) {
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

        ID3D12GraphicsCommandList6 *pCmdList = nullptr;
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
	        if (OptimizeResourceBarrierState(subResourceState)) {
		        subResourceState = D3D12_RESOURCE_STATE_COMMON;
	        }
        }
        if (OptimizeResourceBarrierState(pResourceState->state)) {
	        pResourceState->state = D3D12_RESOURCE_STATE_COMMON;
        }
    }

    GlobalResourceState::UnLock();
}

}    // namespace dx
