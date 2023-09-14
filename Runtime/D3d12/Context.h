#pragma once
#include "D3dUtils.h"
#include "ResourceStateTracker.h"
#include "Foundation/ReadonlyArraySpan.hpp"
#include <glm/glm.hpp>
namespace dx {
class Context : private NonCopyable {
protected:
    friend class FrameResource;
    Context(ID3D12GraphicsCommandList6 *pCommandList);
    virtual ~Context();
public:
    void Transition(ID3D12Resource *pResource,
        D3D12_RESOURCE_STATES stateAfter,
        UINT subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
        D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE);

    void FlushResourceBarriers();
    auto GetCommandList() const -> ID3D12GraphicsCommandList6 *;
    virtual auto GetContextType() const -> ContextType = 0;
protected:
    // clang-format off
	ID3D12GraphicsCommandList6 *_pCommandList;
	ResourceStateTracker		_resourceStateTracker;
    // clang-format on
};
class ComputeContext : public Context {
public:
    ComputeContext(ID3D12GraphicsCommandList6 *pCommandList);
    ~ComputeContext() override;
public:
    auto GetContextType() const -> ContextType override {
        return ContextType::eCompute;
    }
};

class GraphicsContext : public ComputeContext {
public:
    GraphicsContext(ID3D12GraphicsCommandList6 *pCommandList);
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

inline Context::Context(ID3D12GraphicsCommandList6 *pCommandList) : _pCommandList(pCommandList) {
}

inline Context::~Context() {
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

#pragma endregion

#pragma region ComputeContext
inline ComputeContext::ComputeContext(ID3D12GraphicsCommandList6 *pCommandList) : Context(pCommandList) {
}

inline ComputeContext::~ComputeContext() {
}
#pragma endregion


#pragma region GraphicsContext
inline GraphicsContext::GraphicsContext(ID3D12GraphicsCommandList6 *pCommandList) : ComputeContext(pCommandList) {
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
