#include "TriangleRenderer.h"
#include "D3d12/Context.h"
#include "D3d12/FrameResource.h"
#include "D3d12/FrameResourceRing.h"
#include "D3d12/StaticBuffer.h"
#include "D3d12/SwapChain.h"
#include "Foundation/GameTimer.h"
#include "Foundation/Logger.h"

void TriangleRenderer::OnCreate(uint32_t numBackBuffer, HWND hwnd) {
	Renderer::OnCreate(numBackBuffer, hwnd);

    _pTriangleStaticBuffer = std::make_shared<dx::StaticBuffer>();
    uint16_t indices[] = {
        0, 1, 2
    };

    float depthValue = 1.0;
    float offset = 0.7f;
    glm::vec3 vertices[] = {
        { 0, -offset, depthValue },
        { -offset, offset, depthValue },
        { offset, offset, depthValue }
    };

    _pTriangleStaticBuffer->OnCreate(_pDevice.get(), sizeof(indices) + sizeof(vertices));
    dx::StaticBufferUploadHeap uploadHeap(*_pTriangleStaticBuffer, *_pUploadHeap);
    _vertexBufferView = uploadHeap.AllocVertexBuffer(std::size(vertices), sizeof(glm::vec3), vertices).value();
    _indexBufferView = uploadHeap.AllocIndexBuffer(std::size(indices), sizeof(uint16_t), indices).value();
    uploadHeap.DoUpload();
}

void TriangleRenderer::OnDestroy() {
	_pTriangleStaticBuffer->OnDestroy();
	Renderer::OnDestroy();
}

void TriangleRenderer::OnRender(GameTimer &timer) {
	Renderer::OnRender(timer);

	static uint64_t frameCount = 0;
    if (static_cast<uint64_t>(timer.GetTotalTime()) > frameCount) {
        Logger::Info("fps {}", timer.GetFPS());
        frameCount = static_cast<uint64_t>(timer.GetTotalTime());
    }

    _pFrameResourceRing->OnBeginFrame();
    dx::FrameResource &frameResource = _pFrameResourceRing->GetCurrentFrameResource();
    std::shared_ptr<dx::GraphicsContext> pGraphicsCtx = frameResource.AllocGraphicsContext();

    CD3DX12_VIEWPORT viewport = CD3DX12_VIEWPORT(0.f, 0.f, static_cast<float>(_width), static_cast<float>(_height));
    D3D12_RECT scissor = {0, 0, static_cast<LONG>(_width), static_cast<LONG>(_height)};
    pGraphicsCtx->SetViewport(viewport);
    pGraphicsCtx->SetScissor(scissor);
    pGraphicsCtx->SetRenderTargets(_pSwapChain->GetCurrentBackBufferRTV());
    pGraphicsCtx->Transition(_pSwapChain->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET);
    pGraphicsCtx->FlushResourceBarriers();

    glm::vec4 color = {
        std::sin(timer.GetTotalTime()) * 0.5f + 0.5f,
        std::cos(timer.GetTotalTime()) * 0.5f + 0.5f,
        0.f,
        1.f,
    };

    pGraphicsCtx->ClearRenderTargetView(_pSwapChain->GetCurrentBackBufferRTV(), color);
    pGraphicsCtx->Transition(_pSwapChain->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT);
    frameResource.ExecuteContexts(pGraphicsCtx.get());

    _pFrameResourceRing->OnEndFrame();
    _pSwapChain->Present();
}
