#include "SwapChain.h"
#include "Device.h"
#include "DescriptorManager.hpp"
#include "ResourceStateTracker.h"
#include "Foundation/StringUtil.h"

namespace dx {

SwapChain::SwapChain() {
}

SwapChain::~SwapChain() {
}

void SwapChain::OnCreate(Device *pDevice, uint32_t numBackBuffers, HWND hwnd, DXGI_FORMAT format) {
    _pDevice = pDevice;
    _numBackBufferCount = numBackBuffers;
    _hwnd = hwnd;

    _swapChainDesc = {};
    _swapChainDesc.BufferCount = _numBackBufferCount;
    _swapChainDesc.Width = 0;
    _swapChainDesc.Height = 0;
    _swapChainDesc.Format = format;
    _swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    _swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    _swapChainDesc.SampleDesc.Count = 1;
    _swapChainDesc.Flags = 0;

    CreateDXGIFactory1(IID_PPV_ARGS(&_pFactory));

    WRL::ComPtr<IDXGISwapChain1> pSwapChain;
    ThrowIfFailed(_pFactory->CreateSwapChainForHwnd(
        _pDevice->GetGraphicsQueue(),    // Swap chain needs the queue so that it can force a flush on it.
        _hwnd,
        &_swapChainDesc,
        nullptr,
        nullptr,
        &pSwapChain));

    ThrowIfFailed(_pFactory->MakeWindowAssociation(_hwnd, DXGI_MWA_NO_ALT_ENTER));
    ThrowIfFailed(pSwapChain->QueryInterface(__uuidof(IDXGISwapChain4), &_pSwapChain));
    _rtvViews = _pDevice->AllocDescriptor<RTV>(numBackBuffers);
}

void SwapChain::OnDestroy() {
    _rtvViews.Release();
}

void SwapChain::OnResize(uint32_t width, uint32_t height) {
    _renderTargetResources.clear();
    ThrowIfFailed(_pSwapChain->ResizeBuffers(_swapChainDesc.BufferCount,
        width,
        height,
        _swapChainDesc.Format,
        _swapChainDesc.Flags));
    CreateRtv();
}

void SwapChain::Present() {
    UINT syncInterval = _bVSyncOn ? 1 : 0;
    ThrowIfFailed(_pSwapChain->Present(syncInterval, 0));
}

auto SwapChain::GetCurrentBackBuffer() const -> ID3D12Resource * {
    uint32_t backBufferIndex = _pSwapChain->GetCurrentBackBufferIndex();
    return _renderTargetResources[backBufferIndex].Get();
}

auto SwapChain::GetCurrentBackBufferRTV() const -> D3D12_CPU_DESCRIPTOR_HANDLE {
    uint32_t backBufferIndex = _pSwapChain->GetCurrentBackBufferIndex();
    return _rtvViews.GetCpuHandle(backBufferIndex);
}

auto SwapChain::GetFormat() const -> DXGI_FORMAT {
    return _swapChainDesc.Format;
}

auto SwapChain::GetHWND() const -> HWND {
    return _hwnd;
}

void SwapChain::SetVSync(bool bVSync) {
    _bVSyncOn = bVSync;
}

void SwapChain::CreateRtv() {
    for (WRL::ComPtr<ID3D12Resource> &pResource : _renderTargetResources) {
        GlobalResourceState::RemoveResourceState(pResource.Get());
    }

    _renderTargetResources.resize(_swapChainDesc.BufferCount);
    ID3D12Device *pDevice = _pDevice->GetNativeDevice();
    for (uint32_t i = 0; i < _swapChainDesc.BufferCount; i++) {
        WRL::ComPtr<ID3D12Resource> pBackBuffer;
        ThrowIfFailed(_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer)));
        D3D12_RENDER_TARGET_VIEW_DESC colorDesc = {};
        colorDesc.Format = _swapChainDesc.Format;
        colorDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        colorDesc.Texture2D.MipSlice = 0;
        colorDesc.Texture2D.PlaneSlice = 0;
        pDevice->CreateRenderTargetView(pBackBuffer.Get(), &colorDesc, _rtvViews.GetCpuHandle(i));
        pBackBuffer->SetName(nstd::to_wstring(fmt::format("SwapChainBuffer_{}", i)).c_str());
        _renderTargetResources[i] = pBackBuffer;
        GlobalResourceState::SetResourceState(pBackBuffer.Get(), D3D12_RESOURCE_STATE_COMMON);
    }
}

}    // namespace dx