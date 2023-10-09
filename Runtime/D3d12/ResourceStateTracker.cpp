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

// 下面这些状态,能够被 D3D12_RESOURCE_STATE_COMMON 隐式转换, 在 ExecuteCommandLists 后, 也能够自动转化为 D3D12_RESOURCE_STATE_COMMON
bool ResourceStateTracker::OptimizeResourceBarrierState(D3D12_RESOURCE_STATES state) {
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

void ResourceStateTracker::FlushResourceBarriers(NativeCommandList *pCommandList) {
	UINT numBarrier = _resourceBarriers.size();
	if (numBarrier > 0) {
		pCommandList->ResourceBarrier(numBarrier, _resourceBarriers.data());
		_resourceBarriers.clear();
	}
}

void ResourceStateTracker::Reset() {
	_pendingResourceBarriers.clear();
	_resourceBarriers.clear();
	_finalResourceState.clear();
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
		currentState = iter->second;
	return currentState;
}

void GlobalResourceState::Lock() {
	_isLock = true;
	_mutex.lock();
}

void GlobalResourceState::UnLock() {
	_isLock = false;
	_mutex.unlock();
}

bool GlobalResourceState::IsLock() {
	return _isLock;
}

void GlobalResourceState::SetResourceState(ID3D12Resource *pResource, ResourceState state) {
	if (pResource != nullptr) {
		std::lock_guard lock(_mutex);
		_resourceStateMap[pResource] = std::move(state);
	}
}

void GlobalResourceState::RemoveResourceState(ID3D12Resource *pResource) {
	if (pResource != nullptr) {
		std::lock_guard lock(_mutex);
		_resourceStateMap.erase(pResource);
	}
}

auto GlobalResourceState::FindResourceState(ID3D12Resource *pResource) -> ResourceState * {
	Assert(IsLock());
	auto iter = _resourceStateMap.find(pResource);
	if (iter != _resourceStateMap.end())
		return &iter->second;
	return nullptr;
}

}    // namespace dx
