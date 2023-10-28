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
    _pSwapChain->OnCreate(_pDevice.get(), 3, hwnd, DXGI_FORMAT_R8G8B8A8_UNORM);

    dx::FrameResourceRingDesc frameResourceRingDesc;
    frameResourceRingDesc.pDevice = _pDevice.get();
    frameResourceRingDesc.numComputeCmdListPreFrame = 1;
    frameResourceRingDesc.numGraphicsCmdListPreFrame = 1;
    frameResourceRingDesc.numFrameResource = 2;
    _pFrameResourceRing->OnCreate(frameResourceRingDesc);
    _pUploadHeap->OnCreate(_pDevice.get(), dx::GetMByte(64));
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

void Renderer::OnRender(GameTimer &timer) {
    ITick::OnRender(timer);
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
