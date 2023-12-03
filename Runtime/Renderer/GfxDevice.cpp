#include "GfxDevice.h"
#include "D3d12/D3dUtils.h"
#include "D3d12/Device.h"
#include "D3d12/SwapChain.h"
#include "D3d12/UploadHeap.h"
#include "D3d12/ASBuilder.h"

GfxDevice::GfxDevice(): _numBackBuffer(0) {
}

GfxDevice::~GfxDevice() {
}

void GfxDevice::OnCreate(uint32_t numBackBuffer, HWND hwnd) {
	_numBackBuffer = numBackBuffer;
    _pDevice = std::make_unique<dx::Device>();
    _pSwapChain = std::make_unique<dx::SwapChain>();
    _pUploadHeap = std::make_unique<dx::UploadHeap>();
    _pASBuilder = std::make_unique<dx::ASBuilder>();

    _pDevice->OnCreate(true);
    _pSwapChain->OnCreate(_pDevice.get(), 3, hwnd, DXGI_FORMAT_R8G8B8A8_UNORM);
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
