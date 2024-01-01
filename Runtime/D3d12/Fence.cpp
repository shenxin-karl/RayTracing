#include "Fence.h"
#include "Device.h"
#include "Foundation/StringUtil.h"

namespace dx {

Fence::Fence() : _event(nullptr), _fenceCounter(0) {
}

Fence::~Fence() {
}

void Fence::OnCreate(Device *pDevice, std::string_view name) {
    _fenceCounter = 0;
    ThrowIfFailed(pDevice->GetNativeDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_pFence)));
    _pFence->SetName(nstd::to_wstring(name).c_str());
    _event = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
}

void Fence::OnDestroy() {
    if (_event != nullptr) {
        CloseHandle(_event);
        _event = nullptr;
        _pFence = nullptr;
    }
}

auto Fence::IssueFence(ID3D12CommandQueue *pCommandQueue) -> uint64_t {
    _fenceCounter++;
    ThrowIfFailed(pCommandQueue->Signal(_pFence.Get(), _fenceCounter));
    return _fenceCounter;
}

void Fence::CpuWaitForFence() const {
    CpuWaitForFence(_fenceCounter);
}

void Fence::CpuWaitForFence(uint64_t waitFenceValue) const {
    UINT64 completedValue = _pFence->GetCompletedValue();
    if (completedValue < waitFenceValue) {
        ThrowIfFailed(_pFence->SetEventOnCompletion(waitFenceValue, _event));
        WaitForSingleObject(_event, INFINITE);
    }
}

void Fence::GpuWaitForFence(ID3D12CommandQueue *pCommandQueue) const {
    ThrowIfFailed(pCommandQueue->Wait(_pFence.Get(), _fenceCounter));
}

void Fence::SetName(std::string_view name) {
    std::wstring wstring = nstd::to_wstring(name);
    _pFence->SetName(wstring.c_str());
}
}    // namespace dx