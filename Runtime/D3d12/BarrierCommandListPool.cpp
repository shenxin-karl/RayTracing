#include "BarrierCommandListPool.h"
#include "Device.h"

namespace dx {

BarrierCommandListPool::BarrierCommandListPool() : _pDevice(nullptr), _currentGraphicsCommandListIndex(0) {
#if ENABLE_D3D_COMPUTE_QUEUE
    _currentComputeCommandListIndex = 0;
#endif
}

BarrierCommandListPool::~BarrierCommandListPool() {
}

void BarrierCommandListPool::OnCreate(Device *pDevice) {
    _pDevice = pDevice;
    ID3D12Device *device = pDevice->GetNativeDevice();
    ThrowIfFailed(
        device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_pGraphicsCommandAllocator)));
    _pGraphicsCommandAllocator->SetName(L"GraphicsBarrierCommandAllocator");

#if ENABLE_D3D_COMPUTE_QUEUE
    ThrowIfFailed(
        device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&_pComputeCommandAllocator)));
    _pComputeCommandAllocator->SetName(L"ComputeBarrierCommandAllocator");

#endif
}

void BarrierCommandListPool::OnDestroy() {
    _pGraphicsCommandAllocator = nullptr;
    _graphicsCommandListPool.clear();
#if ENABLE_D3D_COMPUTE_QUEUE
    _pComputeCommandAllocator = nullptr;
    _computeCommandListPool.clear();
#endif
}

void BarrierCommandListPool::OnBeginFrame() {
    ThrowIfFailed(_pGraphicsCommandAllocator->Reset());
    for (size_t i = 0; i < _currentGraphicsCommandListIndex; ++i) {
    }
    _currentGraphicsCommandListIndex = 0;

#if ENABLE_D3D_COMPUTE_QUEUE
    ThrowIfFailed(_pComputeCommandAllocator->Reset());
    for (size_t i = 0; i < _currentComputeCommandListIndex; ++i) {
        ThrowIfFailed(_computeCommandListPool[i]->Reset(_pGraphicsCommandAllocator.Get(), nullptr));
    }
    _currentComputeCommandListIndex = 0;
#endif
}

auto BarrierCommandListPool::AllocGraphicsCommandList() -> NativeCommandList * {
    ID3D12Device *device = _pDevice->GetNativeDevice();
	size_t index = _currentGraphicsCommandListIndex++;
    if (index >= _graphicsCommandListPool.size()) {
        WRL::ComPtr<NativeCommandList> pCmdList = nullptr;
        ThrowIfFailed(device->CreateCommandList(0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            _pGraphicsCommandAllocator.Get(),
            nullptr,
            IID_PPV_ARGS(&pCmdList)));
        _graphicsCommandListPool.push_back(pCmdList);
        pCmdList->SetName(L"GraphicsBarrierCommandList");
        return pCmdList.Get();
    } else {
	    ThrowIfFailed(_graphicsCommandListPool[index]->Reset(_pGraphicsCommandAllocator.Get(), nullptr));
	    return _graphicsCommandListPool[index].Get();
    }

}

#if ENABLE_D3D_COMPUTE_QUEUE
auto BarrierCommandListPool::AllocComputeCommandList() -> NativeCommandList * {
    ID3D12Device *device = _pDevice->GetNativeDevice();
    if (_currentComputeCommandListIndex >= _computeCommandListPool.size()) {
        WRL::ComPtr<NativeCommandList> pCmdList = nullptr;
        ThrowIfFailed(device->CreateCommandList(0,
            D3D12_COMMAND_LIST_TYPE_COMPUTE,
            _pComputeCommandAllocator.Get(),
            nullptr,
            IID_PPV_ARGS(&pCmdList)));
        _computeCommandListPool.push_back(pCmdList);
        pCmdList->SetName(L"ComputeBarrierCommandList");
    }
    return _computeCommandListPool[_currentComputeCommandListIndex++].Get();
}
#endif

}    // namespace dx
