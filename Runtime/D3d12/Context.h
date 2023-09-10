#pragma once
#include "D3dUtils.h"
#include "ResourceStateTracker.h"
#include "Foundation/ReadonlyArraySpan.hpp"
#include <glm/glm.hpp>

namespace dx {

#pragma region Context
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
#pragma endregion

#pragma region ComputeContext
class ComputeContext : public Context {
public:
    ComputeContext(ID3D12GraphicsCommandList6 *pCommandList);
    ~ComputeContext() override;
public:
    auto GetContextType() const -> ContextType override {
        return ContextType::eCompute;
    }
};
#pragma endregion

#pragma region GraphicsContext
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
#pragma endregion

}    // namespace dx
