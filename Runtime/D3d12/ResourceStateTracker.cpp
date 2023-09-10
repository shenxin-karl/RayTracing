#include "ResourceStateTracker.h"

namespace dx {

void ResourceStateTracker::Transition(ID3D12Resource *pResource,
    D3D12_RESOURCE_STATES stateAfter,
    UINT subResource,
    D3D12_RESOURCE_BARRIER_FLAGS flags) {

    D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(pResource,
        D3D12_RESOURCE_STATE_COMMON,
        stateAfter,
        subResource,
        flags);
	ResourceBarrier(barrier);
}

void ResourceStateTracker::ResourceBarrier(D3D12_RESOURCE_BARRIER barrier) {
	if (barrier.Type != D3D12_RESOURCE_BARRIER_TYPE_TRANSITION) {
		_resourceBarriers.push_back(barrier);
		return;
	}

	ID3D12Resource *pResource = barrier.Transition.pResource;
	UINT subResource = barrier.Transition.Subresource;
	D3D12_RESOURCE_STATES stateAfter = barrier.Transition.StateAfter;
    auto iter = _finalResourceState.find(pResource);
    if (iter != _finalResourceState.end()) {
	    ResourceState &resourceState = iter->second;
        if (subResource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES && !resourceState.subResourceStateMap.empty()) {
	        for (auto &&[subResource, currentState] : resourceState.subResourceStateMap) {
				if (currentState != stateAfter) {
					D3D12_RESOURCE_BARRIER subResourceBarrier = barrier;
					subResourceBarrier.Transition.Subresource = subResource;
                    subResourceBarrier.Transition.StateBefore = currentState;
					_resourceBarriers.push_back(subResourceBarrier);
				}
			}
        } else {
	        auto lastResourceState = resourceState.GetSubResourceState(subResource);
            barrier.Transition.StateBefore = lastResourceState;
			_resourceBarriers.push_back(barrier);
        }
    } else {
		_pendingResourceBarriers.push_back(barrier);
    }
    _finalResourceState[pResource].SetSubResourceState(subResource, stateAfter);
}

void ResourceStateTracker::FlushResourceBarriers(ID3D12GraphicsCommandList6 *pCommandList) {
	UINT numBarrier = _resourceBarriers.size();
	if (numBarrier > 0) {
		pCommandList->ResourceBarrier(numBarrier, _resourceBarriers.data());
		_resourceBarriers.clear();
	}
}

void ResourceStateTracker::FlushPendingResourceBarriers(ID3D12GraphicsCommandList6 *pCommandList) {

}

ResourceStateTracker::ResourceState::ResourceState(D3D12_RESOURCE_STATES state) : state(state) {
}

void ResourceStateTracker::ResourceState::SetSubResourceState(UINT subResource, D3D12_RESOURCE_STATES currentState) {
	if (subResource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) {
		state = currentState;
		subResourceStateMap.clear();
	} else {
		subResourceStateMap[subResource] = currentState;
	}
}

auto ResourceStateTracker::ResourceState::GetSubResourceState(UINT subResource) -> D3D12_RESOURCE_STATES {
	D3D12_RESOURCE_STATES currentState = state;
	if (auto iter = subResourceStateMap.find(subResource); iter != subResourceStateMap.end())
		state = iter->second;
	return state;
}

}    // namespace dx
