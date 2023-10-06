#pragma once
#include <vector>
#include "D3dUtils.h"
#include "DescriptorHandle.h"
#include "Fence.h"

namespace dx {

class Device;
class SwapChain : NonCopyable {
public:
    SwapChain();
    ~SwapChain();
public:
    void OnCreate(Device *pDevice, uint32_t numBackBuffers, HWND hwnd, DXGI_FORMAT format);
    void OnDestroy();
    void OnResize(uint32_t width, uint32_t height);
    void Present();
    auto GetCurrentBackBuffer() const -> ID3D12Resource *;
    auto GetCurrentBackBufferRTV() const -> D3D12_CPU_DESCRIPTOR_HANDLE;
    auto GetFormat() const -> DXGI_FORMAT;
    void SetVSync(bool bVSync);
private:
    void CreateRtv();
private:
    // clang-format off
    HWND                                        _hwnd = nullptr;
    bool                                        _bVSyncOn = false;
    Device *                                    _pDevice = nullptr;
    uint32_t                                    _numBackBufferCount = 0;
    DXGI_SWAP_CHAIN_DESC1                       _swapChainDesc = {};
    WRL::ComPtr<IDXGIFactory6>                  _pFactory = nullptr;
    WRL::ComPtr<IDXGISwapChain4>                _pSwapChain = nullptr;
    RTV                                         _rtvViews;
    std::vector<WRL::ComPtr<ID3D12Resource>>    _renderTargetResources = {};
    // clang-format on
};

}    // namespace dx