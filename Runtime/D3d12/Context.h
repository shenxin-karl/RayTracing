#pragma once
#include "D3dUtils.h"
#include "ResourceStateTracker.h"
#include "DynamicBufferAllocator.h"
#include "DynamicDescriptorHeap.h"
#include "Foundation/ReadonlyArraySpan.hpp"
#include <glm/glm.hpp>

#include "DescriptorHandle.h"

namespace dx {

class Context : NonCopyable {
protected:
    friend class FrameResource;
    Context(Device *pDevice);
    virtual ~Context();
    void Reset(ID3D12GraphicsCommandList6 *pCommandList);
public:
    void Transition(ID3D12Resource *pResource,
        D3D12_RESOURCE_STATES stateAfter,
        UINT subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
        D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE);

    void FlushResourceBarriers();
    auto GetCommandList() const -> ID3D12GraphicsCommandList6 *;

    void SetDynamicViews(size_t rootIndex, size_t numDescriptors, const DescriptorHandle &handle, size_t offset = 0);
    void SetDynamicSamples(size_t rootIndex, size_t numDescriptors, const DescriptorHandle &handle, size_t offset = 0);

    auto AllocVertexBuffer(size_t numOfVertices, size_t strideInBytes, const void *pInitData)
        -> D3D12_VERTEX_BUFFER_VIEW;
    auto AllocIndexBuffer(size_t numOfIndex, size_t strideInBytes, const void *pInitData) -> D3D12_INDEX_BUFFER_VIEW;
    auto AllocConstantBuffer(size_t strideInBytes, const void *pInitData) -> D3D12_GPU_VIRTUAL_ADDRESS;
    auto AllocStructuredBuffer(size_t numOfVertices, size_t strideInBytes, const void *pInitData)
        -> D3D12_GPU_VIRTUAL_ADDRESS;
public:
    virtual auto GetContextType() const -> ContextType = 0;
protected:
    // clang-format off
	ID3D12GraphicsCommandList6 *_pCommandList;
	ResourceStateTracker		_resourceStateTracker;
    DynamicBufferAllocator      _dynamicBufferAllocator;
    DynamicDescriptorHeap       _viewDynamicHeap;
    DynamicDescriptorHeap       _sampleDynamicHeap;
    // clang-format on
};

class ComputeContext : public Context {
public:
    ComputeContext(Device *pDevice);
    ~ComputeContext() override;
public:
    auto GetContextType() const -> ContextType override {
        return ContextType::eCompute;
    }
};

class GraphicsContext : public ComputeContext {
public:
    GraphicsContext(Device *pDevice);
    ~GraphicsContext() override;
public:
    void SetViewport(ReadonlyArraySpan<D3D12_VIEWPORT> viewports);
    void SetScissor(ReadonlyArraySpan<D3D12_RECT> scissors);
    void SetRenderTargets(ReadonlyArraySpan<D3D12_CPU_DESCRIPTOR_HANDLE> renderTargets);
    void SetRenderTargets(ReadonlyArraySpan<D3D12_CPU_DESCRIPTOR_HANDLE> renderTargets,
        D3D12_CPU_DESCRIPTOR_HANDLE depthDescriptor);
    void ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE rtv,
        glm::vec4 color,
        ReadonlyArraySpan<D3D12_RECT> rects = {});
public:
    auto GetContextType() const -> ContextType override {
        return ContextType::eGraphics;
    }
};

#pragma region Context

inline Context::Context(Device *pDevice)
    : _pCommandList(nullptr),
      _viewDynamicHeap(pDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128),
      _sampleDynamicHeap(pDevice, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 32) {

    _dynamicBufferAllocator.OnCreate(pDevice);
}

inline Context::~Context() {
    _dynamicBufferAllocator.OnDestroy();
}

inline void Context::Reset(ID3D12GraphicsCommandList6 *pCommandList) {
    _pCommandList = pCommandList;
    _resourceStateTracker.Reset();
    _dynamicBufferAllocator.Reset();
}

inline void Context::Transition(ID3D12Resource *pResource,
    D3D12_RESOURCE_STATES stateAfter,
    UINT subResource,
    D3D12_RESOURCE_BARRIER_FLAGS flags) {

    D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(pResource,
        D3D12_RESOURCE_STATE_COMMON,
        stateAfter,
        subResource,
        flags);
    _resourceStateTracker.ResourceBarrier(barrier);
}

inline void Context::FlushResourceBarriers() {
    _resourceStateTracker.FlushResourceBarriers(_pCommandList);
}

inline auto Context::GetCommandList() const -> ID3D12GraphicsCommandList6 * {
    return _pCommandList;
}

inline void Context::SetDynamicViews(size_t rootIndex,
    size_t numDescriptors,
    const DescriptorHandle &handle,
    size_t offset) {

    _viewDynamicHeap.StageDescriptors(rootIndex, numDescriptors, handle.GetCpuHandle(), offset);
}

inline void Context::SetDynamicSamples(size_t rootIndex, size_t numDescriptors, const DescriptorHandle &handle,
	size_t offset) {
    _sampleDynamicHeap.StageDescriptors(rootIndex, numDescriptors, handle.GetCpuHandle(), offset);
}

inline auto Context::AllocVertexBuffer(size_t numOfVertices, size_t strideInBytes, const void *pInitData)
    -> D3D12_VERTEX_BUFFER_VIEW {
    return _dynamicBufferAllocator.AllocVertexBuffer(numOfVertices, strideInBytes, pInitData);
}

inline auto Context::AllocIndexBuffer(size_t numOfIndex, size_t strideInBytes, const void *pInitData)
    -> D3D12_INDEX_BUFFER_VIEW {
    return _dynamicBufferAllocator.AllocIndexBuffer(numOfIndex, strideInBytes, pInitData);
}

inline auto Context::AllocConstantBuffer(size_t strideInBytes, const void *pInitData) -> D3D12_GPU_VIRTUAL_ADDRESS {
    return _dynamicBufferAllocator.AllocConstantBuffer(strideInBytes, pInitData);
}

inline auto Context::AllocStructuredBuffer(size_t numOfVertices, size_t strideInBytes, const void *pInitData)
    -> D3D12_GPU_VIRTUAL_ADDRESS {
    return _dynamicBufferAllocator.AllocStructuredBuffer(numOfVertices, strideInBytes, pInitData);
}

#pragma endregion

#pragma region ComputeContext
inline ComputeContext::ComputeContext(Device *pDevice) : Context(pDevice) {
}

inline ComputeContext::~ComputeContext() {
}
#pragma endregion

#pragma region GraphicsContext
inline GraphicsContext::GraphicsContext(Device *pDevice) : ComputeContext(pDevice) {
}

inline GraphicsContext::~GraphicsContext() {
}

inline void GraphicsContext::SetViewport(ReadonlyArraySpan<D3D12_VIEWPORT> viewports) {
    _pCommandList->RSSetViewports(viewports.Count(), viewports.Data());
}

inline void GraphicsContext::SetScissor(ReadonlyArraySpan<D3D12_RECT> scissors) {
    _pCommandList->RSSetScissorRects(scissors.Count(), scissors.Data());
}

inline void GraphicsContext::SetRenderTargets(ReadonlyArraySpan<D3D12_CPU_DESCRIPTOR_HANDLE> renderTargets) {
    _pCommandList->OMSetRenderTargets(renderTargets.Count(), renderTargets.Data(), false, nullptr);
}

inline void GraphicsContext::SetRenderTargets(ReadonlyArraySpan<D3D12_CPU_DESCRIPTOR_HANDLE> renderTargets,
    D3D12_CPU_DESCRIPTOR_HANDLE depthDescriptor) {

    _pCommandList->OMSetRenderTargets(renderTargets.Count(), renderTargets.Data(), false, &depthDescriptor);
}

inline void GraphicsContext::ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE rtv,
    glm::vec4 color,
    ReadonlyArraySpan<D3D12_RECT> rects) {

    float c[4] = {color.r, color.g, color.b, color.a};
    _pCommandList->ClearRenderTargetView(rtv, c, rects.Count(), rects.Data());
}

#pragma endregion

}    // namespace dx
