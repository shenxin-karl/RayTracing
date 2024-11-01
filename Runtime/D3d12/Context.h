#pragma once
#include "D3dStd.h"
#include "ResourceStateTracker.h"
#include "DynamicBufferAllocator.h"
#include "DynamicDescriptorHeap.h"
#include "Foundation/ReadonlyArraySpan.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "DescriptorHandle.h"
#include "RootSignature.h"
#include "ShaderRecode.h"

namespace dx {

class Context : NonCopyable {
protected:
    friend class FrameResource;
    Context(Device *pDevice);
    virtual ~Context();
    void Reset(NativeCommandList *pCommandList, ID3D12CommandAllocator *pCommandAllocator);
public:
    void Transition(ID3D12Resource *pResource,
        D3D12_RESOURCE_STATES stateAfter,
        UINT subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
        D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE);

    void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, ID3D12DescriptorHeap *pDescriptorHeap);
    void BindDescriptorHeaps();

    // Calling other SDK methods may cause the bound descriptor heap to be modified.
    // This method is to re-attach the dynamic descriptor heap
    void BindDynamicDescriptorHeap();
    void FlushResourceBarriers();
    void CopyResource(ID3D12Resource *pDstResource, ID3D12Resource *pSrcResource);
    auto GetCommandList() const -> NativeCommandList *;
    auto GetCommandAllocator() const -> ID3D12CommandAllocator *;

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
    auto AllocBuffer(size_t sizeInByte, size_t addressAlignment = 1) -> DynamicBufferAllocator::AllocInfo;

    template<typename T>
    auto AllocConstantBuffer(const T &data) -> D3D12_GPU_VIRTUAL_ADDRESS {
        return AllocConstantBuffer(sizeof(T), &data);
    }
public:
    virtual auto GetContextType() const -> ContextType = 0;
protected:
    // clang-format off
	NativeCommandList          *_pCommandList;
    ID3D12CommandAllocator     *_pCommandAllocator;
	ResourceStateTracker		_resourceStateTracker;
    DynamicBufferAllocator      _dynamicBufferAllocator;
    DynamicDescriptorHeap       _dynamicViewDescriptorHeap;
    DynamicDescriptorHeap       _dynamicSampleDescriptorHeap;
    ID3D12DescriptorHeap       *_bindDescriptorHeap[2];
    // clang-format on
};

struct DispatchRaysDesc {
    ShaderRecode rayGenerationShaderRecode;
    std::vector<ShaderRecode> missShaderTable;
    std::vector<ShaderRecode> hitGroupTable;
    std::vector<ShaderRecode> callShaderTable;
    UINT width;
    UINT height;
    UINT depth;
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
    void Dispatch(UINT groupX, UINT groupY, UINT groupZ);

    void ClearUnorderedAccessViewFloat(ID3D12Resource *pResource,
        D3D12_CPU_DESCRIPTOR_HANDLE cpuUAV,
        glm::vec4 clearValues,
        ReadonlyArraySpan<D3D12_RECT> rects = {});

    template<typename T>
    void SetComputeRootDynamicConstantBuffer(UINT rootIndex, const T &data);

    void SetRayTracingPipelineState(ID3D12StateObject *pStateObject);
    void DispatchRays(const DispatchRaysDesc &dispatchRaysDesc);
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
    void SetRenderTargets(D3D12_CPU_DESCRIPTOR_HANDLE baseHandle,
        UINT renderTargetCount,
        D3D12_CPU_DESCRIPTOR_HANDLE depthDescriptor);
    void ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE rtv,
        glm::vec4 color,
        ReadonlyArraySpan<D3D12_RECT> rects = {});
    void ClearDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE dsv,
        D3D12_CLEAR_FLAGS clearFlags,
        float depth,
        UINT8 stencil,
        ReadonlyArraySpan<D3D12_RECT> rects = {});
    void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology);
    void SetVertexBuffers(UINT startSlot, ReadonlyArraySpan<D3D12_VERTEX_BUFFER_VIEW> views);
    void SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW &view);
    void DrawInstanced(UINT vertexCountPreInstance,
        UINT instanceCount,
        UINT startVertexLocation,
        UINT startInstanceLocation);
    void DrawIndexedInstanced(UINT indexCountPreInstance,
        UINT instanceCount,
        UINT startIndexLocation,
        UINT baseVertexLocation,
        UINT startInstanceLocation);
    void SetGraphicsRootSignature(RootSignature *pRootSignature);
    void SetGraphics32Constant(UINT rootIndex, DWParam val, UINT offset = 0);
    void SetGraphics32Constants(UINT rootIndex, ReadonlyArraySpan<DWParam> span, UINT offset = 0);
    void SetGraphics32Constants(UINT rootIndex, UINT num32BitValuesToSet, const void *pData, UINT offset = 0);
    void SetGraphicsRootConstantBufferView(UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferLocation);
    void SetGraphicsRootShaderResourceView(UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferLocation);
    void SetGraphicsRootUnorderedAccessView(UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferLocation);

    void SetBlendFactor(const float blendFactor[4]);
    void SetBlendFactor(glm::vec4 blendFactor);

    template<typename T>
    void SetGraphicsRootDynamicConstantBuffer(UINT rootIndex, const T &data);
public:
    auto GetContextType() const -> ContextType override {
        return ContextType::eGraphics;
    }
};

#pragma region Context

inline Context::Context(Device *pDevice)
    : _pCommandList(nullptr),
      _pCommandAllocator(nullptr),
      _dynamicViewDescriptorHeap(pDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kDynamicDescriptorMaxView),
      _dynamicSampleDescriptorHeap(pDevice, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, kDynamicDescriptorMaxSampler),
      _bindDescriptorHeap{} {
    _dynamicBufferAllocator.OnCreate(pDevice);
}

inline Context::~Context() {
    _dynamicBufferAllocator.OnDestroy();
}

inline void Context::Reset(NativeCommandList *pCommandList, ID3D12CommandAllocator *pCommandAllocator) {
    _pCommandList = pCommandList;
    _pCommandAllocator = pCommandAllocator;
    _resourceStateTracker.Reset();
    _dynamicBufferAllocator.Reset();
    _dynamicViewDescriptorHeap.Reset();
    _dynamicSampleDescriptorHeap.Reset();
    _bindDescriptorHeap[0] = nullptr;
    _bindDescriptorHeap[1] = nullptr;
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

inline void Context::SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, ID3D12DescriptorHeap *pDescriptorHeap) {
    Assert(type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    _bindDescriptorHeap[type] = pDescriptorHeap;
}

inline void Context::BindDescriptorHeaps() {
    ID3D12DescriptorHeap *heaps[2];
    size_t count = 0;
    for (size_t i = 0; i < 2; ++i) {
        if (_bindDescriptorHeap[i] != nullptr) {
            heaps[count++] = _bindDescriptorHeap[i];
        }
    }
    if (count > 0) {
        _pCommandList->SetDescriptorHeaps(count, heaps);
    }
}

inline void Context::BindDynamicDescriptorHeap() {
    _dynamicViewDescriptorHeap.BindDescriptorHeap(_pCommandList);
    _dynamicSampleDescriptorHeap.BindDescriptorHeap(_pCommandList);
}

inline void Context::FlushResourceBarriers() {
    _resourceStateTracker.FlushResourceBarriers(_pCommandList);
}

inline void Context::CopyResource(ID3D12Resource *pDstResource, ID3D12Resource *pSrcResource) {
    FlushResourceBarriers();
    _pCommandList->CopyResource(pDstResource, pSrcResource);
}

inline auto Context::GetCommandList() const -> NativeCommandList * {
    return _pCommandList;
}

inline auto Context::GetCommandAllocator() const -> ID3D12CommandAllocator * {
    return _pCommandAllocator;
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

inline auto Context::AllocBuffer(size_t sizeInByte, size_t addressAlignment) -> DynamicBufferAllocator::AllocInfo {
    return _dynamicBufferAllocator.AllocBuffer(sizeInByte, addressAlignment);
}

#pragma endregion

#pragma region ComputeContext
inline ComputeContext::ComputeContext(Device *pDevice) : Context(pDevice) {
}

inline ComputeContext::~ComputeContext() {
}

inline void ComputeContext::SetComputeRootSignature(RootSignature *pRootSignature) {
    _dynamicViewDescriptorHeap.ParseRootSignature(pRootSignature);
    _dynamicSampleDescriptorHeap.ParseRootSignature(pRootSignature);
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

inline void ComputeContext::Dispatch(UINT groupX, UINT groupY, UINT groupZ) {
    FlushResourceBarriers();
    _dynamicViewDescriptorHeap.CommitStagedDescriptorForDispatch(this);
    _dynamicSampleDescriptorHeap.CommitStagedDescriptorForDispatch(this);
    _pCommandList->Dispatch(groupX, groupY, groupZ);
}

template<typename T>
void ComputeContext::SetComputeRootDynamicConstantBuffer(UINT rootIndex, const T &data) {
    D3D12_GPU_VIRTUAL_ADDRESS bufferLoc = _dynamicBufferAllocator.AllocConstantBuffer(sizeof(T), &data);
    _pCommandList->SetComputeRootConstantBufferView(rootIndex, bufferLoc);
}

inline void ComputeContext::SetRayTracingPipelineState(ID3D12StateObject *pStateObject) {
#if ENABLE_RAY_TRACING
    _pCommandList->SetPipelineState1(pStateObject);
#endif
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
inline void GraphicsContext::SetRenderTargets(D3D12_CPU_DESCRIPTOR_HANDLE baseHandle,
    UINT renderTargetCount,
    D3D12_CPU_DESCRIPTOR_HANDLE depthDescriptor) {
    _pCommandList->OMSetRenderTargets(renderTargetCount, &baseHandle, true, &depthDescriptor);
}

inline void GraphicsContext::ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE rtv,
    glm::vec4 color,
    ReadonlyArraySpan<D3D12_RECT> rects) {

    _pCommandList->ClearRenderTargetView(rtv, glm::value_ptr(color), rects.Count(), rects.Data());
}

inline void GraphicsContext::ClearDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE dsv,
    D3D12_CLEAR_FLAGS clearFlags,
    float depth,
    UINT8 stencil,
    ReadonlyArraySpan<D3D12_RECT> rects) {

    _pCommandList->ClearDepthStencilView(dsv, clearFlags, depth, stencil, rects.Count(), rects.Data());
}

inline void GraphicsContext::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology) {
    _pCommandList->IASetPrimitiveTopology(topology);
}

inline void GraphicsContext::SetVertexBuffers(UINT startSlot, ReadonlyArraySpan<D3D12_VERTEX_BUFFER_VIEW> views) {
    _pCommandList->IASetVertexBuffers(startSlot, views.Count(), views.Data());
}

inline void GraphicsContext::SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW &view) {
    _pCommandList->IASetIndexBuffer(&view);
}

inline void GraphicsContext::DrawInstanced(UINT vertexCountPreInstance,
    UINT instanceCount,
    UINT startVertexLocation,
    UINT startInstanceLocation) {
    FlushResourceBarriers();
    _dynamicViewDescriptorHeap.CommitStagedDescriptorForDraw(this);
    _dynamicSampleDescriptorHeap.CommitStagedDescriptorForDraw(this);
    _pCommandList->DrawInstanced(vertexCountPreInstance, instanceCount, startVertexLocation, startInstanceLocation);
}

inline void GraphicsContext::DrawIndexedInstanced(UINT indexCountPreInstance,
    UINT instanceCount,
    UINT startIndexLocation,
    UINT baseVertexLocation,
    UINT startInstanceLocation) {
    FlushResourceBarriers();
    _dynamicViewDescriptorHeap.CommitStagedDescriptorForDraw(this);
    _dynamicSampleDescriptorHeap.CommitStagedDescriptorForDraw(this);
    _pCommandList->DrawIndexedInstanced(indexCountPreInstance,
        instanceCount,
        startIndexLocation,
        baseVertexLocation,
        startInstanceLocation);
}

inline void GraphicsContext::SetGraphicsRootSignature(RootSignature *pRootSignature) {
    _dynamicViewDescriptorHeap.ParseRootSignature(pRootSignature);
    _dynamicSampleDescriptorHeap.ParseRootSignature(pRootSignature);
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

inline void GraphicsContext::SetBlendFactor(const float blendFactor[4]) {
    _pCommandList->OMSetBlendFactor(blendFactor);
}

inline void GraphicsContext::SetBlendFactor(glm::vec4 blendFactor) {
    _pCommandList->OMSetBlendFactor(glm::value_ptr(blendFactor));
}

template<typename T>
void GraphicsContext::SetGraphicsRootDynamicConstantBuffer(UINT rootIndex, const T &data) {
    D3D12_GPU_VIRTUAL_ADDRESS bufferLoc = _dynamicBufferAllocator.AllocConstantBuffer(sizeof(T), &data);
    _pCommandList->SetGraphicsRootConstantBufferView(rootIndex, bufferLoc);
}

#pragma endregion

}    // namespace dx
