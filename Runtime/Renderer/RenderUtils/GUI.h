#pragma once
#include "Foundation/NonCopyable.h"
#include "D3d12/D3dStd.h"
#include <Windows.h>

namespace dx {
class GraphicsContext;
}

class GUI : private NonCopyable {
public:
    void OnCreate();
    void OnDestroy();
    bool PollEvent(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void NewFrame();
    // before call in SwapChain::Present
    void Render();
public:
    static auto Get() -> GUI &;
private:
    struct FrameContext {
        dx::WRL::ComPtr<ID3D12CommandAllocator> pCommandAllocator = nullptr;
        UINT64 fenceValue = 0;
    };
    bool InitD3DObjects();
private:
    // clang-format off
    ID3D12CommandQueue                           *_pGraphicsQueue = nullptr;
    dx::WRL::ComPtr<ID3D12DescriptorHeap>         _pSrvDescHeap = nullptr;
    std::vector<FrameContext>                     _frameContexts;
    uint32_t                                      _frameIndex = 0;
    dx::WRL::ComPtr<ID3D12GraphicsCommandList>    _pCommandList;
    std::unique_ptr<dx::Fence>                    _pFence;
    // clang-format on
};
