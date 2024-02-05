#include "Device.h"
#include "DescriptorManager.hpp"
#include "Fence.h"
#include "Foundation/StringUtil.h"

namespace dx {

Device::Device(): _pAllocator(nullptr), _workGroupWarpSize(32) {
}

Device::~Device() {
}

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

    DXGI_ADAPTER_DESC adapterDesc = {};
    ThrowIfFailed(_pAdapter->GetDesc(&adapterDesc));

    std::wstring adapterDescription = adapterDesc.Description;
    adapterDescription = nstd::tolower(adapterDescription);
    if (adapterDescription.find(L"AMD") != std::wstring::npos) {
	    _workGroupWarpSize = 64;
    }

    ThrowIfFailed(D3D12CreateDevice(_pAdapter.Get(), KD3D_FEATURE_LEVEL, IID_PPV_ARGS(&_pDevice)));
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

    _pCopyQueueFence = std::make_unique<Fence>();
    _pCopyQueueFence->OnCreate(this, "CopyQueueFence");

    D3D12_COMMAND_QUEUE_DESC directQueueDesc = {};
    directQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    directQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    directQueueDesc.NodeMask = 0;
    ThrowIfFailed(_pDevice->CreateCommandQueue(&directQueueDesc, IID_PPV_ARGS(&_pDirectQueue)));
    _pDirectQueue->SetName(L"DirectQueue");

    _pDirectQueueFence = std::make_unique<Fence>();
    _pDirectQueueFence->OnCreate(this, "DirectQueueFence");

#if ENABLE_D3D_COMPUTE_QUEUE
    D3D12_COMMAND_QUEUE_DESC computeQueueDesc = {};
    computeQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    computeQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
    computeQueueDesc.NodeMask = 0;
    ThrowIfFailed(_pDevice->CreateCommandQueue(&computeQueueDesc, IID_PPV_ARGS(&_pComputeQueue)));
    _pComputeQueue->SetName(L"ComputeQueue");

    _pComputeQueueFence = std::make_unique<Fence>();
    _pComputeQueueFence->OnCreate(this, "ComputeQueueFence");
#endif

    D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
    allocatorDesc.pDevice = _pDevice.Get();
    allocatorDesc.pAdapter = _pAdapter.Get();
    ThrowIfFailed(CreateAllocator(&allocatorDesc, &_pAllocator));

    _pDescriptorManager = std::make_unique<DescriptorManager>();
    _pDescriptorManager->OnCreate(this);
}

void Device::OnDestroy() {
    _pDescriptorManager->OnDestroy();
    _pDescriptorManager = nullptr;

    _pAllocator->Release();
    _pAllocator = nullptr;
#if ENABLE_D3D_COMPUTE_QUEUE
    _pComputeQueue = nullptr;
#endif
    _pDirectQueue = nullptr;
    _pCopyQueue = nullptr;
    _pDevice = nullptr;
    _pAdapter = nullptr;
}

void Device::WaitForGPUFlush(D3D12_COMMAND_LIST_TYPE queueType) {
    ID3D12CommandQueue *pQueue = nullptr;
    Fence *pFence = nullptr;

    switch (queueType) {
#if ENABLE_D3D_COMPUTE_QUEUE
    case D3D12_COMMAND_LIST_TYPE_COMPUTE:
        pQueue = GetComputeQueue();
        pFence = _pComputeQueueFence.get();
        break;
#endif
    case D3D12_COMMAND_LIST_TYPE_DIRECT:
        pQueue = GetGraphicsQueue();
        pFence = _pDirectQueueFence.get();
        break;
    case D3D12_COMMAND_LIST_TYPE_COPY:
        pQueue = GetCopyQueue();
        pFence = _pCopyQueueFence.get();
        break;
    default:
        return;
    }

    pFence->IssueFence(pQueue);
    pFence->CpuWaitForFence();
}

void Device::WaitForGPUFlush() {
    WaitForGPUFlush(D3D12_COMMAND_LIST_TYPE_COPY);
    WaitForGPUFlush(D3D12_COMMAND_LIST_TYPE_COMPUTE);
    WaitForGPUFlush(D3D12_COMMAND_LIST_TYPE_DIRECT);
}

void Device::ReleaseStaleDescriptors() {
    _pDescriptorManager->ReleaseStaleDescriptors();
}

auto Device::AllocDescriptorInternal(size_t numDescriptor, D3D12_DESCRIPTOR_HEAP_TYPE type) -> DescriptorHandle {
    return _pDescriptorManager->Alloc(numDescriptor, type);
}

}    // namespace dx