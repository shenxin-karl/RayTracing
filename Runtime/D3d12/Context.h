#pragma once
#include "D3dUtils.h"
#include "ResourceStateTracker.h"
#include "DynamicBufferAllocator.h"
#include "DynamicDescriptorHeap.h"
#include "Foundation/ReadonlyArraySpan.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "DescriptorHandle.h"
#include "RootSignature.h"

namespace dx {

struct DWParam {
    DWParam(float f) : Float(f) {
    }
    DWParam(uint32_t u) : Uint(u) {
    }
    DWParam(int32_t i) : Int(i) {
    }
    void operator=(float f) {
        Float = f;
    }
    void operator=(uint32_t u) {
        Uint = u;
    }
    void operator=(int32_t i) {
        Int = i;
    }

    union {
        float Float;
        uint32_t Uint;
        int32_t Int;
    };
};

class Context : NonCopyable {
protected:
    friend class FrameResource;
    Context(Device *pDevice);
    virtual ~Context();
    void Reset(NativeCommandList *pCommandList);
public:
    void Transition(ID3D12Resource *pResource,
        D3D12_RESOURCE_STATES stateAfter,
        UINT subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
        D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE);

    void FlushResourceBarriers();
    auto GetCommandList() const -> NativeCommandList *;

    void SetPipelineState(ID3D12PipelineState *pPipelineState);
    void SetDynamicViews(size_t rootIndex, size_t numDescriptors, const DescriptorHandle &handle, size_t offset = 0);
    void SetDynamicViews(size_t rootIndex, ReadonlyArraySpan<D3D12_CPU_DESCRIPTOR_HANDLE> handles, size_t offset = 0);
    void SetDynamicSamples(size_t rootIndex, size_t numDescriptors, const DescriptorHandle &handle, size_t offset = 0);
    void SetDynamicSamples(size_t rootIndex, ReadonlyArraySpan<D3D12_CPU_DESCRIPTOR_HANDLE> handles, size_t offset = 0);

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
	NativeCommandList          *_pCommandList;
	ResourceStateTracker		_resourceStateTracker;
    DynamicBufferAllocator      _dynamicBufferAllocator;
    DynamicDescriptorHeap       _dynamicViewDescriptorHeap;
    DynamicDescriptorHeap       _dynamicSampleDescriptorHeap;
    // clang-format on
};

class ComputeContext : public Context {
public:
    ComputeContext(Device *pDevice);
    ~ComputeContext() override;
public:
    void SetComputeRootSignature(RootSignature *pRootSignature);
    void SetCompute32Constant(UINT rootIndex, DWParam val, UINT offset = 0);
    void SetCompute32Constants(UINT rootIndex, ReadonlyArraySpan<DWParam> span, UINT offset = 0);
    void SetCompute32Constants(UINT rootIndex, UINT num32BitValuesToSet, const void *pData, UINT offset = 0);
    void SetComputeRootConstantBufferView(UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferLocation);
    void SetComputeRootShaderResourceView(UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferLocation);
    void SetComputeRootUnorderedAccessView(UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferLocation);
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
    void SetVertexBuffers(UINT startSlot, ReadonlyArraySpan<D3D12_VERTEX_BUFFER_VIEW> views);
    void SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW &view);
    void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology);
    void SetGraphicsRootSignature(RootSignature *pRootSignature);
    void SetGraphics32Constant(UINT rootIndex, DWParam val, UINT offset = 0);
    void SetGraphics32Constants(UINT rootIndex, ReadonlyArraySpan<DWParam> span, UINT offset = 0);
    void SetGraphics32Constants(UINT rootIndex, UINT num32BitValuesToSet, const void *pData, UINT offset = 0);
    void SetGraphicsRootConstantBufferView(UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferLocation);
    void SetGraphicsRootShaderResourceView(UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferLocation);
    void SetGraphicsRootUnorderedAccessView(UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferLocation);
public:
    auto GetContextType() const -> ContextType override {
        return ContextType::eGraphics;
    }
};

#pragma region Context

inline Context::Context(Device *pDevice)
    : _pCommandList(nullptr),
      _dynamicViewDescriptorHeap(pDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128),
      _dynamicSampleDescriptorHeap(pDevice, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 32) {

    _dynamicBufferAllocator.OnCreate(pDevice);
}

inline Context::~Context() {
    _dynamicBufferAllocator.OnDestroy();
}

inline void Context::Reset(NativeCommandList *pCommandList) {
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

inline auto Context::GetCommandList() const -> NativeCommandList * {
    return _pCommandList;
}

inline void Context::SetPipelineState(ID3D12PipelineState *pPipelineState) {
    _pCommandList->SetPipelineState(pPipelineState);
}

inline void Context::SetDynamicViews(size_t rootIndex,
                                     size_t numDescriptors,
                                     const DescriptorHandle &handle,
                                     size_t offset) {

    _dynamicViewDescriptorHeap.StageDescriptors(rootIndex, numDescriptors, handle.GetCpuHandle(), offset);
}

inline void Context::SetDynamicViews(size_t rootIndex,
    ReadonlyArraySpan<D3D12_CPU_DESCRIPTOR_HANDLE> handles,
    size_t offset) {
    _dynamicViewDescriptorHeap.StageDescriptors(rootIndex, handles, offset);
}

inline void Context::SetDynamicSamples(size_t rootIndex,
    size_t numDescriptors,
    const DescriptorHandle &handle,
    size_t offset) {
    _dynamicSampleDescriptorHeap.StageDescriptors(rootIndex, numDescriptors, handle.GetCpuHandle(), offset);
}

inline void Context::SetDynamicSamples(size_t rootIndex,
    ReadonlyArraySpan<D3D12_CPU_DESCRIPTOR_HANDLE> handles,
    size_t offset) {
    _dynamicSampleDescriptorHeap.StageDescriptors(rootIndex, handles, offset);
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

inline void ComputeContext::SetComputeRootSignature(RootSignature *pRootSignature) {
    _pCommandList->SetComputeRootSignature(pRootSignature->GetRootSignature());
}

inline void ComputeContext::SetCompute32Constant(UINT rootIndex, DWParam val, UINT offset) {
    _pCommandList->SetComputeRoot32BitConstant(rootIndex, val.Uint, offset);
}

inline void ComputeContext::SetCompute32Constants(UINT rootIndex, ReadonlyArraySpan<DWParam> span, UINT offset) {
    _pCommandList->SetComputeRoot32BitConstants(rootIndex, span.Count(), span.Data(), offset);
}

inline void ComputeContext::SetCompute32Constants(UINT rootIndex,
    UINT num32BitValuesToSet,
    const void *pData,
    UINT offset) {

    _pCommandList->SetComputeRoot32BitConstants(rootIndex, num32BitValuesToSet, pData, offset);
}

inline void ComputeContext::SetComputeRootConstantBufferView(UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferLocation) {
    _pCommandList->SetComputeRootConstantBufferView(rootIndex, bufferLocation);
}

inline void ComputeContext::SetComputeRootShaderResourceView(UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferLocation) {
    _pCommandList->SetComputeRootShaderResourceView(rootIndex, bufferLocation);
}

inline void ComputeContext::SetComputeRootUnorderedAccessView(UINT rootIndex,
    D3D12_GPU_VIRTUAL_ADDRESS bufferLocation) {
    _pCommandList->SetComputeRootUnorderedAccessView(rootIndex, bufferLocation);
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

    _pCommandList->ClearRenderTargetView(rtv, glm::value_ptr(color), rects.Count(), rects.Data());
}

inline void GraphicsContext::SetVertexBuffers(UINT startSlot, ReadonlyArraySpan<D3D12_VERTEX_BUFFER_VIEW> views) {
    _pCommandList->IASetVertexBuffers(startSlot, views.Count(), views.Data());
}

inline void GraphicsContext::SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW &view) {
    _pCommandList->IASetIndexBuffer(&view);
}

inline void GraphicsContext::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology) {
    _pCommandList->IASetPrimitiveTopology(topology);
}

inline void GraphicsContext::SetGraphicsRootSignature(RootSignature *pRootSignature) {
    _pCommandList->SetGraphicsRootSignature(pRootSignature->GetRootSignature());
}

inline void GraphicsContext::SetGraphics32Constant(UINT rootIndex, DWParam val, UINT offset) {
    _pCommandList->SetGraphicsRoot32BitConstant(rootIndex, val.Uint, offset);
}

inline void GraphicsContext::SetGraphics32Constants(UINT rootIndex, ReadonlyArraySpan<DWParam> span, UINT offset) {
    _pCommandList->SetGraphicsRoot32BitConstants(rootIndex, span.Count(), span.Data(), offset);
}

inline void GraphicsContext::SetGraphics32Constants(UINT rootIndex,
    UINT num32BitValuesToSet,
    const void *pData,
    UINT offset) {
    _pCommandList->SetGraphicsRoot32BitConstants(rootIndex, num32BitValuesToSet, pData, offset);
}

inline void GraphicsContext::SetGraphicsRootConstantBufferView(UINT rootIndex,
    D3D12_GPU_VIRTUAL_ADDRESS bufferLocation) {
    _pCommandList->SetGraphicsRootConstantBufferView(rootIndex, bufferLocation);
}

inline void GraphicsContext::SetGraphicsRootShaderResourceView(UINT rootIndex,
    D3D12_GPU_VIRTUAL_ADDRESS bufferLocation) {
    _pCommandList->SetGraphicsRootShaderResourceView(rootIndex, bufferLocation);
}

inline void GraphicsContext::SetGraphicsRootUnorderedAccessView(UINT rootIndex,
    D3D12_GPU_VIRTUAL_ADDRESS bufferLocation) {
    _pCommandList->SetGraphicsRootUnorderedAccessView(rootIndex, bufferLocation);
}

#pragma endregion

}    // namespace dx
