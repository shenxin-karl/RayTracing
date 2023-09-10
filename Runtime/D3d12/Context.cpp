#include "Context.h"

namespace dx {

#pragma region Context

Context::Context(ID3D12GraphicsCommandList6 *pCommandList) : _pCommandList(pCommandList) {
}

Context::~Context() {
}

void Context::Transition(ID3D12Resource *pResource,
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

void Context::FlushResourceBarriers() {
    _resourceStateTracker.FlushResourceBarriers(_pCommandList);
}

auto Context::GetCommandList() const -> ID3D12GraphicsCommandList6 * {
    return _pCommandList;
}

#pragma endregion

#pragma region ComputeContext

ComputeContext::ComputeContext(ID3D12GraphicsCommandList6 *pCommandList) : Context(pCommandList) {
}

ComputeContext::~ComputeContext() {
}

#pragma endregion

#pragma region GraphicsContext

GraphicsContext::GraphicsContext(ID3D12GraphicsCommandList6 *pCommandList) : ComputeContext(pCommandList) {
}

GraphicsContext::~GraphicsContext() {
}

void GraphicsContext::SetViewport(ReadonlyArraySpan<D3D12_VIEWPORT> viewports) {
    _pCommandList->RSSetViewports(viewports.Count(), viewports.Data());
}

void GraphicsContext::SetScissor(ReadonlyArraySpan<D3D12_RECT> scissors) {
    _pCommandList->RSSetScissorRects(scissors.Count(), scissors.Data());
}

void GraphicsContext::SetRenderTargets(ReadonlyArraySpan<D3D12_CPU_DESCRIPTOR_HANDLE> renderTargets) {
    _pCommandList->OMSetRenderTargets(renderTargets.Count(), renderTargets.Data(), false, nullptr);
}

void GraphicsContext::SetRenderTargets(ReadonlyArraySpan<D3D12_CPU_DESCRIPTOR_HANDLE> renderTargets,
    D3D12_CPU_DESCRIPTOR_HANDLE depthDescriptor) {

    _pCommandList->OMSetRenderTargets(renderTargets.Count(), renderTargets.Data(), false, &depthDescriptor);
}

void GraphicsContext::ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE rtv,
    glm::vec4 color,
    ReadonlyArraySpan<D3D12_RECT> rects) {

    float c[4] = {color.r, color.g, color.b, color.a};
    _pCommandList->ClearRenderTargetView(rtv, c, rects.Count(), rects.Data());
}

#pragma endregion

}    // namespace dx
