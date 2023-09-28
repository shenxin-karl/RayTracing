#include "Renderer.h"
#include "D3d12/Context.h"
#include "D3d12/DescriptorManager.hpp"
#include "D3d12/Device.h"
#include "D3d12/FrameResource.h"
#include "D3d12/FrameResourceRing.h"
#include "D3d12/StaticBuffer.h"
#include "D3d12/SwapChain.h"
#include "D3d12/UploadHeap.h"
#include "Foundation/GameTimer.h"
#include "Foundation/Logger.h"

Renderer::Renderer() : _width(0), _height(0), _numBackBuffer(0) {
}

Renderer::~Renderer() {
}

void Renderer::OnCreate(uint32_t numBackBuffer, HWND hwnd) {
    dx::DescriptorManager *pDescriptorManger = dx::DescriptorManager::OnInstanceCreate();

    _numBackBuffer = numBackBuffer;
    _pDevice = std::make_unique<dx::Device>();
    _pSwapChain = std::make_unique<dx::SwapChain>();
    _pFrameResourceRing = std::make_unique<dx::FrameResourceRing>();
    _pUploadHeap = std::make_unique<dx::UploadHeap>();

    _pDevice->OnCreate(true);
    pDescriptorManger->OnCreate(_pDevice.get());
    _pSwapChain->OnCreate(_pDevice.get(), _numBackBuffer, hwnd, DXGI_FORMAT_R8G8B8A8_UNORM);

    dx::FrameResourceRingDesc frameResourceRingDesc;
    frameResourceRingDesc.pDevice = _pDevice.get();
    frameResourceRingDesc.numComputeCmdListPreFrame = 1;
    frameResourceRingDesc.numGraphicsCmdListPreFrame = 1;
    frameResourceRingDesc.numFrameResource = 3;
    _pFrameResourceRing->OnCreate(frameResourceRingDesc);
    _pUploadHeap->OnCreate(_pDevice.get(), dx::GetMByte(64));

    InitTriangleGeometry();
}

void Renderer::OnDestroy() {
    dx::DescriptorManager *pDescriptorManger = dx::DescriptorManager::GetInstance();

    _pDevice->WaitForGPUFlush();
    _pFrameResourceRing->OnDestroy();
    _pUploadHeap->OnDestroy();
    _pSwapChain->OnDestroy();
    pDescriptorManger->OnDestroy();
    _pDevice->OnDestroy();

    dx::DescriptorManager::OnInstanceDestroy();
}

void Renderer::OnPreUpdate(GameTimer &timer) {
    ITick::OnPreUpdate(timer);
}

void Renderer::OnUpdate(GameTimer &timer) {
    ITick::OnUpdate(timer);
}

void Renderer::OnPostUpdate(GameTimer &timer) {
    ITick::OnPostUpdate(timer);
}

void Renderer::OnPreRender(GameTimer &timer) {
    ITick::OnPreRender(timer);
}

void Renderer::OnRender(GameTimer &timer) {
    ITick::OnRender(timer);

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

void Renderer::OnPostRender(GameTimer &timer) {
    ITick::OnPostRender(timer);
    dx::DescriptorManager::GetInstance()->ReleaseStaleDescriptors();
}

void Renderer::OnResize(uint32_t width, uint32_t height) {
    _pDevice->WaitForGPUFlush();
    _width = width;
    _height = height;
    _pSwapChain->OnResize(width, height);
}

void Renderer::InitTriangleGeometry() {
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
