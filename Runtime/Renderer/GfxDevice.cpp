#include "GfxDevice.h"
#include "D3d12/D3dStd.h"
#include "D3d12/Device.h"
#include "D3d12/SwapChain.h"
#include "D3d12/UploadHeap.h"
#include "D3d12/ASBuilder.h"

GfxDevice::GfxDevice()
    : _numBackBuffer(0), _renderTargetFormat(DXGI_FORMAT_UNKNOWN), _depthStencilFormat(DXGI_FORMAT_UNKNOWN) {
}

GfxDevice::~GfxDevice() {
}

void GfxDevice::OnCreate(uint32_t numBackBuffer, HWND hwnd, DXGI_FORMAT rtFormat, DXGI_FORMAT dsFormat) {
    _numBackBuffer = numBackBuffer;
    _renderTargetFormat = rtFormat;
    _depthStencilFormat = dsFormat;

    _pDevice = std::make_unique<dx::Device>();
    _pSwapChain = std::make_unique<dx::SwapChain>();
    _pUploadHeap = std::make_unique<dx::UploadHeap>();
    _pASBuilder = std::make_unique<dx::SyncASBuilder>();

    _pDevice->OnCreate(true);
    _pSwapChain->OnCreate(_pDevice.get(), _numBackBuffer, hwnd, DXGI_FORMAT_R8G8B8A8_UNORM);
    _pUploadHeap->OnCreate(_pDevice.get(), dx::GetMByte(64));
    _pASBuilder->OnCreate(_pDevice.get());
}

void GfxDevice::OnDestroy() {
    _pDevice->WaitForGPUFlush();

    _pASBuilder->OnDestroy();
    _pUploadHeap->OnDestroy();
    _pSwapChain->OnDestroy();
    _pDevice->OnDestroy();

    _pASBuilder = nullptr;
    _pUploadHeap = nullptr;
    _pSwapChain = nullptr;
    _pDevice = nullptr;
}
