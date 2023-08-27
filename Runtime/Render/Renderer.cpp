#include "Renderer.h"
#include "D3d12/CommandListRing.h"
#include "D3d12/DescriptorManager.hpp"
#include "D3d12/Device.h"
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
    _pGraphicsCmdListPool = std::make_unique<dx::CommandListRing>();
    _pUploadHeap = std::make_unique<dx::UploadHeap>();

    _pDevice->OnCreate(true);
	pDescriptorManger->OnCreate(_pDevice.get());
    _pSwapChain->OnCreate(_pDevice.get(), _numBackBuffer, hwnd, DXGI_FORMAT_R8G8B8A8_UNORM);
    _pGraphicsCmdListPool->OnCreate(_pDevice.get(), numBackBuffer + 1, 3, D3D12_COMMAND_LIST_TYPE_DIRECT);
    _pUploadHeap->OnCreate(_pDevice.get(), dx::GetMByte(64));
}

void Renderer::OnDestroy() {
	dx::DescriptorManager *pDescriptorManger = dx::DescriptorManager::GetInstance();

    _pDevice->WaitForGPUFlush();
    _pUploadHeap->OnDestroy();
    _pGraphicsCmdListPool->OnDestroy();
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

    _pGraphicsCmdListPool->OnBeginFrame();

    auto cmd = _pGraphicsCmdListPool->GetNewCommandList();
    CD3DX12_VIEWPORT viewport = CD3DX12_VIEWPORT(0.f, 0.f, static_cast<float>(_width), static_cast<float>(_height));
    D3D12_RECT scissor = {0, 0, static_cast<LONG>(_width), static_cast<LONG>(_height)};
    cmd->RSSetViewports(1, &viewport);
    cmd->RSSetScissorRects(1, &scissor);

    float color[] = {
        std::sin(timer.GetTotalTime()) * 0.5f + 0.5f,
        std::cos(timer.GetTotalTime()) * 0.5f + 0.5f,
        0.f,
        1.f,
    };

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = _pSwapChain->GetCurrentBackBufferRTV();

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(_pSwapChain->GetCurrentBackBuffer(),
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    cmd->ResourceBarrier(1, &barrier);

    cmd->OMSetRenderTargets(1, &rtv, false, nullptr);
    cmd->ClearRenderTargetView(rtv, color, 0, nullptr);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(_pSwapChain->GetCurrentBackBuffer(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT);
    cmd->ResourceBarrier(1, &barrier);

    dx::ThrowIfFailed(cmd->Close());

    ID3D12CommandList *cmdList[] = {cmd};
    _pDevice->GetGraphicsQueue()->ExecuteCommandLists(1, cmdList);

    _pGraphicsCmdListPool->OnEndFrame(_pDevice->GetGraphicsQueue());
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
