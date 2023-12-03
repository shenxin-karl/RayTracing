#include "Renderer.h"
#include "GfxDevice.h"
#include "D3d12/Context.h"
#include "D3d12/Device.h"
#include "D3d12/FrameResourceRing.h"
#include "D3d12/SwapChain.h"
#include "D3d12/UploadHeap.h"
#include "Foundation/GameTimer.h"
#include "Foundation/Logger.h"

Renderer::Renderer() : _width(0), _height(0), _pDevice(nullptr), _pSwapChain(nullptr), _pUploadHeap(nullptr) {
}

Renderer::~Renderer() {
}

void Renderer::OnCreate() {
    GfxDevice *pGfxDevice = GfxDevice::GetInstance();
    _pDevice = pGfxDevice->GetDevice();
    _pSwapChain = pGfxDevice->GetSwapChain();
    _pUploadHeap = pGfxDevice->GetUploadHeap();

    _pFrameResourceRing = std::make_unique<dx::FrameResourceRing>();
    dx::FrameResourceRingDesc frameResourceRingDesc;
    frameResourceRingDesc.pDevice = _pDevice;
    frameResourceRingDesc.numComputeCmdListPreFrame = 1;
    frameResourceRingDesc.numGraphicsCmdListPreFrame = 1;
    frameResourceRingDesc.numFrameResource = 2;
    _pFrameResourceRing->OnCreate(frameResourceRingDesc);
}

void Renderer::OnDestroy() {
    _pFrameResourceRing->OnDestroy();
    _pFrameResourceRing = nullptr;
}

void Renderer::OnPostRender(GameTimer &timer) {
    ITick::OnPostRender(timer);
    _pDevice->ReleaseStaleDescriptors();
}

void Renderer::OnResize(uint32_t width, uint32_t height) {
    _width = width;
    _height = height;
    _pSwapChain->OnResize(width, height);
}
