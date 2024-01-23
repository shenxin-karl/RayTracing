#pragma once
#include <mutex>
#include <unordered_map>
#include "D3dStd.h"

namespace dx {

class ResourceStateTracker : NonCopyable {
public:
    void Transition(ID3D12Resource *pResource,
        D3D12_RESOURCE_STATES stateAfter,
        UINT subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
        D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE);
public:
    struct ResourceState;
    using ResourceBarriers = std::vector<D3D12_RESOURCE_BARRIER>;
    using ResourceStateMap = std::unordered_map<ID3D12Resource *, ResourceState>;
private:
    friend class Context;
    friend class FrameResource;
    void ResourceBarrier(D3D12_RESOURCE_BARRIER barrier);
    void FlushResourceBarriers(NativeCommandList *pCommandList);
    auto GetPendingResourceBarriers() -> ResourceBarriers & {
        return _pendingResourceBarriers;
    }
    auto GetFinalResourceStateMap() const -> const ResourceStateMap & {
        return _finalResourceState;
    }
    void Reset();
private:
    // clang-format off
	ResourceBarriers      _pendingResourceBarriers;
	ResourceBarriers      _resourceBarriers;
	ResourceStateMap      _finalResourceState;
    // clang-format on
};

struct ResourceStateTracker::ResourceState {
    ResourceState(D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON);
    void SetSubResourceState(UINT subResource, D3D12_RESOURCE_STATES currentState);
    auto GetSubResourceState(UINT subResource) -> D3D12_RESOURCE_STATES;
public:
    // clang-format off
	D3D12_RESOURCE_STATES							state;
	std::unordered_map<UINT, D3D12_RESOURCE_STATES> subResourceStateMap;
    // clang-format on
};

class GlobalResourceState : NonCopyable {
public:
    using ResourceState = ResourceStateTracker::ResourceState;
    using ResourceStateMap = ResourceStateTracker::ResourceStateMap;
public:
    static void Lock();
    static void UnLock();
    static bool IsLock();
    static void SetResourceState(ID3D12Resource *pResource, ResourceState state);
    static void RemoveResourceState(ID3D12Resource *pResource);
    static auto FindResourceState(ID3D12Resource *pResource) -> ResourceState *;
private:
    // clang-format off
    static inline bool              _isLock           = false;
    static inline std::mutex        _mutex              = {};
    static inline ResourceStateMap  _resourceStateMap   = {};
    // clang-format on
};

class StateHelper {
public:
    static constexpr bool AllowCommonStatePromotion(D3D12_RESOURCE_STATES state) {
        switch (state) {
        case D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER:
        case D3D12_RESOURCE_STATE_INDEX_BUFFER:
        case D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:
        case D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:
        case D3D12_RESOURCE_STATE_STREAM_OUT:
        case D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT:
        case D3D12_RESOURCE_STATE_COPY_DEST:
        case D3D12_RESOURCE_STATE_COPY_SOURCE:
        case D3D12_RESOURCE_STATE_GENERIC_READ:
            return true;
        default:
            return false;
        }
    }
    static constexpr bool AllowStateDecayToCommon(D3D12_RESOURCE_STATES state) {
        uint64_t stateBit = static_cast<uint64_t>(state);
        D3D12_RESOURCE_STATES stateMask = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER |
                                          D3D12_RESOURCE_STATE_INDEX_BUFFER |
                                          D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
                                          D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_STREAM_OUT |
                                          D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT | D3D12_RESOURCE_STATE_COPY_DEST |
                                          D3D12_RESOURCE_STATE_COPY_SOURCE | D3D12_RESOURCE_STATE_GENERIC_READ;
        return (stateBit & ~stateMask) == 0;
    }

    static constexpr bool IsReadState(D3D12_RESOURCE_STATES state) {
        uint64_t stateBit = static_cast<uint64_t>(state);
        uint64_t genericReadMask = static_cast<uint64_t>(D3D12_RESOURCE_STATE_GENERIC_READ);
        return (stateBit & ~genericReadMask) == 0;
    }
    static constexpr bool IsWriteState(D3D12_RESOURCE_STATES state) {
        return !IsReadState(state);
    }
    static constexpr bool AllowSkippingTransition(D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
        if (IsReadState(before) && IsReadState(after)) {
            return true;
        }
        if (before == D3D12_RESOURCE_STATE_COMMON && AllowCommonStatePromotion(after)) {
            return true;
        }
        return false;
    }
};

}    // namespace dx
