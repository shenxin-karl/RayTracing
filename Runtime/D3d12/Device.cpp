#include "Device.h"

namespace dx {

void Device::OnCreate(bool validationEnabled) {
    if (validationEnabled) {
        WRL::ComPtr<ID3D12Debug1> pDebugController;
        if (D3D12GetDebugInterface(IID_PPV_ARGS(&pDebugController)) == S_OK) {
            // Enabling GPU Validation without enabling the debug layer does nothing
            if (validationEnabled) {
                pDebugController->EnableDebugLayer();
            }
        }
    }

    UINT factoryFlags = 0;
    if (validationEnabled)
        factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

    WRL::ComPtr<IDXGIFactory> pFactory;
    WRL::ComPtr<IDXGIFactory6> pFactory6;
    ThrowIfFailed(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&pFactory)));
    if (S_OK == pFactory->QueryInterface(IID_PPV_ARGS(&pFactory6))) {
        ThrowIfFailed(
            pFactory6->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&_pAdapter)));
    } else {
        ThrowIfFailed(pFactory->EnumAdapters(0, &_pAdapter));
    }

    ThrowIfFailed(D3D12CreateDevice(_pAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&_pDevice)));
    if (validationEnabled) {
        WRL::ComPtr<ID3D12InfoQueue> pInfoQueue;
        if (_pDevice->QueryInterface(IID_PPV_ARGS(&pInfoQueue)) == S_OK) {
            pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
            pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
            pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
        }
    }
    _pDevice->SetName(L"Device");

    // Use the direct type to create the copy queue because the resource barriers directive is to be executed
    D3D12_COMMAND_QUEUE_DESC copyQueue = {};
    copyQueue.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    copyQueue.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;        
    copyQueue.NodeMask = 0;
    ThrowIfFailed(_pDevice->CreateCommandQueue(&copyQueue, IID_PPV_ARGS(&_pCopyQueue)));
    _pCopyQueue->SetName(L"CopyQueue");

    D3D12_COMMAND_QUEUE_DESC directQueueDesc = {};
    directQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    directQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    directQueueDesc.NodeMask = 0;
    ThrowIfFailed(_pDevice->CreateCommandQueue(&directQueueDesc, IID_PPV_ARGS(&_pDirectQueue)));
    _pDirectQueue->SetName(L"DirectQueue");

#if ENABLE_D3D_COMPUTE_QUEUE
    D3D12_COMMAND_QUEUE_DESC computeQueueDesc = {};
    computeQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    computeQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
    computeQueueDesc.NodeMask = 0;
    ThrowIfFailed(_pDevice->CreateCommandQueue(&computeQueueDesc, IID_PPV_ARGS(&_pComputeQueue)));
    _pComputeQueue->SetName(L"ComputeQueue");
#endif

    D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
    allocatorDesc.pDevice = _pDevice.Get();
    allocatorDesc.pAdapter = _pAdapter.Get();
    ThrowIfFailed(CreateAllocator(&allocatorDesc, &_pAllocator));
}

void Device::OnDestroy() {
    _pAllocator->Release();
    _pAllocator = nullptr;
}

void Device::WaitForGPUFlush(D3D12_COMMAND_LIST_TYPE queueType) {
    ID3D12CommandQueue *queue = nullptr;
    switch (queueType) {
#if ENABLE_D3D_COMPUTE_QUEUE
    case D3D12_COMMAND_LIST_TYPE_COMPUTE:
        queueType = GetComputeQueue();
        break;
#endif
    case D3D12_COMMAND_LIST_TYPE_DIRECT:
        queue = GetGraphicsQueue();
        break;
    case D3D12_COMMAND_LIST_TYPE_COPY:
        queue = GetCopyQueue();
        break;
    default:
        return;
    }

    WRL::ComPtr<ID3D12Fence> pFence;
    ThrowIfFailed(_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence)));
    ThrowIfFailed(queue->Signal(pFence.Get(), 1));

    HANDLE mHandleFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    pFence->SetEventOnCompletion(1, mHandleFenceEvent);
    WaitForSingleObject(mHandleFenceEvent, INFINITE);
    CloseHandle(mHandleFenceEvent);
}

void Device::WaitForGPUFlush() {
    WaitForGPUFlush(D3D12_COMMAND_LIST_TYPE_COPY);
    WaitForGPUFlush(D3D12_COMMAND_LIST_TYPE_COMPUTE);
    WaitForGPUFlush(D3D12_COMMAND_LIST_TYPE_DIRECT);
}

}    // namespace dx