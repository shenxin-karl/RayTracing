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
    _barrierCommandListPool.OnCreate(pDevice);

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
    _barrierCommandListPool.OnBeginFrame();
    _pGraphicsCmdListPool->OnBeginFrame();
    _graphicsContextIndex = 0;
#if ENABLE_D3D_COMPUTE_QUEUE
    _computeContextIndex = 0;
#endif
}

auto FrameResource::AllocGraphicsContext() -> std::shared_ptr<GraphicsContext> {
    Assert(_graphicsContextIndex < _graphicsContextList.size());
    auto pContext = _graphicsContextList[_graphicsContextIndex++];
    pContext->Reset(_pGraphicsCmdListPool->AllocCommandList(), _pGraphicsCmdListPool->GetCommandAllocator());
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
    using ResourceStateMap = ResourceStateTracker::ResourceStateMap;

    GlobalResourceState::Lock();

    std::vector<ID3D12CommandList *> graphicsCmdList;
    std::vector<ID3D12CommandList *> computeCmdList;
    ResourceBarriers linkCommandListStateBarriers;

    for (int i = 0; i < static_cast<int>(contexts.Count()); ++i) {
        ResourceStateTracker &resourceStateTracker = contexts[i]->_resourceStateTracker;
        ResourceBarriers &pendingResourceBarriers = resourceStateTracker.GetPendingResourceBarriers();
        for (D3D12_RESOURCE_BARRIER barrier : pendingResourceBarriers) {
            ID3D12Resource *pResource = barrier.Transition.pResource;
            ResourceState *pGlobalResourceStateRecode = GlobalResourceState::FindResourceState(pResource);
            assert(pGlobalResourceStateRecode != nullptr);

            // translation all sub resource to after state
            if (barrier.Transition.Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) {
                if (pGlobalResourceStateRecode->subResourceStateMap.empty() &&
                    barrier.Transition.StateAfter == pGlobalResourceStateRecode->state) {
                    continue;
                }

                D3D12_RESOURCE_STATES currentState = pGlobalResourceStateRecode->state;
                pGlobalResourceStateRecode->state = barrier.Transition.StateAfter;
#if 0
                if (pGlobalResourceStateRecode->subResourceStateMap.empty() && currentState == D3D12_RESOURCE_STATE_COMMON &&
                    StateHelper::AllowSkippingTransition(currentState, barrier.Transition.StateAfter)) {
                    continue;
                }
#endif

                if (pGlobalResourceStateRecode->subResourceStateMap.empty()) {
                    barrier.Transition.StateBefore = currentState;
                    linkCommandListStateBarriers.push_back(barrier);
                    continue;
                }

                for (auto &&[subResource, subResourceState] : pGlobalResourceStateRecode->subResourceStateMap) {
                    if (barrier.Transition.StateAfter != subResourceState) {
                        barrier.Transition.Subresource = subResource;
                        barrier.Transition.StateBefore = subResourceState;
                        linkCommandListStateBarriers.push_back(barrier);
                    }
                }
                continue;
            }

            D3D12_RESOURCE_STATES currentState = pGlobalResourceStateRecode->GetSubResourceState(barrier.Transition.Subresource);
            if (currentState != barrier.Transition.StateAfter) {
                barrier.Transition.StateBefore = currentState;
                pGlobalResourceStateRecode->state = barrier.Transition.StateAfter;
                linkCommandListStateBarriers.push_back(barrier);
            }
        }

        const ResourceStateMap &finalResourceMap = resourceStateTracker.GetFinalResourceStateMap();
        for (auto &&[pResource, resourceState] : finalResourceMap) {
	        auto *pGlobalResourceState = GlobalResourceState::FindResourceState(pResource);
            *pGlobalResourceState = resourceState;
        }

        pendingResourceBarriers.clear();
        if (linkCommandListStateBarriers.empty()) {
            continue;
        }

        // The first command list, there is no command list to execute ResourceBarrier so we need to allocate one
        NativeCommandList *pCmdList = nullptr;
        if (i == 0) {                           
            if (contexts[i]->GetContextType() == ContextType::eGraphics) {
				pCmdList = _barrierCommandListPool.AllocGraphicsCommandList();
				pCmdList->ResourceBarrier(linkCommandListStateBarriers.size(), linkCommandListStateBarriers.data());
	            graphicsCmdList.push_back(pCmdList);
	        } else {
                #if ENABLE_D3D_COMPUTE_QUEUE
					pCmdList = _barrierCommandListPool.AllocComputeCommandList();
					computeCmdList.push_back(pCmdList);
				#endif
	        }
            ThrowIfFailed(pCmdList->Close());
        } else {
            pCmdList = contexts[i - 1]->GetCommandList();
            pCmdList->ResourceBarrier(linkCommandListStateBarriers.size(), linkCommandListStateBarriers.data());
     
        }
    }

    for (Context *pContext : contexts) {
        pContext->FlushResourceBarriers();
        ThrowIfFailed(pContext->GetCommandList()->Close());
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

#if 0
    // After command list is executed, the resource state is implicitly transformed
    for (Context *pContext : contexts) {
	    const auto &finalResourceStateMap = pContext->_resourceStateTracker.GetFinalResourceStateMap();
        for (auto &&[pResource, resourceState] : finalResourceStateMap) {
            ResourceState *pGlobalResourceStateRecode = GlobalResourceState::FindResourceState(pResource);
		    if (resourceState.subResourceStateMap.empty()) {
	            D3D12_RESOURCE_STATES state = resourceState.state;
	            if (StateHelper::AllowStateDecayToCommon(resourceState.state)) {
	                state = D3D12_RESOURCE_STATE_COMMON;
	            }
	            pGlobalResourceStateRecode->SetSubResourceState(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, state);
	            continue;
	        }
	        for (auto [subResource, subResourceState] : resourceState.subResourceStateMap) {
	            if (StateHelper::AllowStateDecayToCommon(resourceState.state)) {
	                subResourceState = D3D12_RESOURCE_STATE_COMMON;
	            }
	            pGlobalResourceStateRecode->SetSubResourceState(subResource, subResourceState);
	        }
        }
    }
#endif

    GlobalResourceState::UnLock();
}

}    // namespace dx
