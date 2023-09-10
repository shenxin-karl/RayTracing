#pragma once
#include <unordered_map>
#include "D3dUtils.h"

namespace dx {

class ResourceStateTracker : NonCopyable {
public:
    void Transition(ID3D12Resource *pResource,
        D3D12_RESOURCE_STATES stateAfter,
        UINT subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
        D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE);
private:
    struct ResourceState;
    using ResourceBarriers = std::vector<D3D12_RESOURCE_BARRIER>;
    using ResourceStateMap = std::unordered_map<ID3D12Resource *, ResourceState>;
private:
    friend class Context;
    friend class FrameResource;
    void ResourceBarrier(D3D12_RESOURCE_BARRIER barrier);
    void FlushResourceBarriers(ID3D12GraphicsCommandList6 *pCommandList);
    void FlushPendingResourceBarriers(ID3D12GraphicsCommandList6 *pCommandList);
private:
    // clang-format off
	ResourceBarriers      _pendingResourceBarriers;
	ResourceBarriers      _resourceBarriers;
	ResourceStateMap      _finalResourceState;
    // clang-format on
};

struct ResourceStateTracker::ResourceState {
    explicit ResourceState(D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON);
    void SetSubResourceState(UINT subResource, D3D12_RESOURCE_STATES currentState);
    auto GetSubResourceState(UINT subResource) -> D3D12_RESOURCE_STATES;
public:
    // clang-format off
	D3D12_RESOURCE_STATES							state;
	std::unordered_map<UINT, D3D12_RESOURCE_STATES> subResourceStateMap;
    // clang-format on
};

}    // namespace dx
