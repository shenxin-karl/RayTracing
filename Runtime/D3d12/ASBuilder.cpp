#include "ASBuilder.h"
#include "Device.h"
#include <glm/gtc/type_ptr.inl>
#include "AccelerationStructure.h"

namespace dx {

AsyncASBuilder::AsyncASBuilder(): _idle(true) {
}

AsyncASBuilder::~AsyncASBuilder() {
    AsyncASBuilder::OnDestroy();
}

void AsyncASBuilder::OnCreate(Device *pDevice) {
    _pDevice = pDevice;
    ID3D12Device *device = pDevice->GetNativeDevice();
    device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_pCommandAllocator));
    device->CreateCommandList(0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        _pCommandAllocator.Get(),
        nullptr,
        IID_PPV_ARGS(&_pCommandList));

    _pCommandAllocator->SetName(L"AsyncASBuilder::CommandListAllocator");
    _pCommandList->SetName(L"AsyncASBuilder::CommandList");

    ThrowIfFailed(_pCommandList->Close());
    ID3D12CommandList *cmdList[] = {_pCommandList.Get()};
    _pDevice->GetCopyQueue()->ExecuteCommandLists(1, cmdList);
    _pDevice->WaitForGPUFlush(D3D12_COMMAND_LIST_TYPE_COPY);

    ThrowIfFailed(_pCommandAllocator->Reset());
    ThrowIfFailed(_pCommandList->Reset(_pCommandAllocator.Get(), nullptr));

    _uploadFinishedFence.OnCreate(pDevice, "AsyncASBuilder::UploadFinishedFence");
}

void AsyncASBuilder::OnDestroy() {
    _pDevice = nullptr;
    _pScratchBuffer = nullptr;
    _pCommandList = nullptr;
    _pCommandAllocator = nullptr;
    _pInstanceBuffer = nullptr;
    _uploadFinishedFence.OnDestroy();
}

void AsyncASBuilder::Flush() {
#if ENABLE_RAY_TRACING
    if (_bottomAsBuildItems.empty() && _topAsBuildItems.empty()) {
        return;
    }

    Assert(IsIdle());
    size_t instanceCount = 0;
    size_t scratchBufferSize = 0;
    for (BottomASBuildItem &bottomBuildItem : _bottomAsBuildItems) {
        scratchBufferSize = std::max(scratchBufferSize, bottomBuildItem.scratchBufferSize);
    }

    for (TopASBuildItem &topBuildItem : _topAsBuildItems) {
        scratchBufferSize = std::max(scratchBufferSize, topBuildItem.scratchBufferSize);
        instanceCount += topBuildItem.instances.size();
    }

    ConditionalGrowInstanceBuffer(instanceCount);
    ConditionalGrowScratchBuffer(scratchBufferSize);

    D3D12_GPU_VIRTUAL_ADDRESS scratchBufferAddress = _pScratchBuffer->GetResource()->GetGPUVirtualAddress();
    for (BottomASBuildItem &bottomBuildItem : _bottomAsBuildItems) {
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
        buildDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
        buildDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        buildDesc.Inputs.NumDescs = bottomBuildItem.vertexBuffers.size();
        buildDesc.Inputs.pGeometryDescs = bottomBuildItem.vertexBuffers.data();
        buildDesc.Inputs.Flags = bottomBuildItem.flags;
        buildDesc.DestAccelerationStructureData = bottomBuildItem.pOutputResource->GetGPUVirtualAddress();
        buildDesc.ScratchAccelerationStructureData = scratchBufferAddress;
        buildDesc.SourceAccelerationStructureData = 0;

        _pCommandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);
        _pCommandList->ResourceBarrier(1, RVPtr(CD3DX12_RESOURCE_BARRIER::UAV(bottomBuildItem.pOutputResource)));
    }
    _bottomAsBuildItems.clear();

    D3D12_RAYTRACING_INSTANCE_DESC *pBuffer = nullptr;
    _pInstanceBuffer->GetResource()->Map(0, nullptr, reinterpret_cast<void **>(&pBuffer));
    D3D12_GPU_VIRTUAL_ADDRESS instanceBufferAddress = _pInstanceBuffer->GetResource()->GetGPUVirtualAddress();

    for (TopASBuildItem &topBuildItem : _topAsBuildItems) {
        D3D12_RAYTRACING_INSTANCE_DESC *pInstance = pBuffer;
        for (ASInstance &instance : topBuildItem.instances) {
            pInstance->InstanceID = instance.instanceID;
            pInstance->InstanceContributionToHitGroupIndex = instance.hitGroupIndex;
            pInstance->Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
            pInstance->AccelerationStructure = instance.pBottomLevelAs->GetGPUVirtualAddress();
            glm::mat4x4 matrix = glm::transpose(instance.transform);
            std::memcpy(pInstance->Transform, glm::value_ptr(matrix), sizeof(pInstance->Transform));
            pInstance->InstanceMask = instance.instanceMask;
            ++pInstance;
        }
        topBuildItem.desc.ScratchAccelerationStructureData = scratchBufferAddress;
        topBuildItem.desc.Inputs.InstanceDescs = instanceBufferAddress;
        topBuildItem.instances.clear();
        _pCommandList->BuildRaytracingAccelerationStructure(&topBuildItem.desc, 0, nullptr);
        _pCommandList->ResourceBarrier(1, RVPtr(CD3DX12_RESOURCE_BARRIER::UAV(topBuildItem.pOutputResource)));
    }

    _topAsBuildItems.clear();
    _pInstanceBuffer->GetResource()->Unmap(0, nullptr);

    ThrowIfFailed(_pCommandList->Close());
    ID3D12CommandList *cmdList[] = {_pCommandList.Get()};
    _pDevice->GetCopyQueue()->ExecuteCommandLists(1, cmdList);

    _uploadFinishedFence.IssueFence(_pDevice->GetCopyQueue());
    _idle = false;
#endif
}

void AsyncASBuilder::Reset() {
    Assert(!IsIdle());
    ThrowIfFailed(_pCommandAllocator->Reset());
    ThrowIfFailed(_pCommandList->Reset(_pCommandAllocator.Get(), nullptr));
    _traceResourceList.clear();
    _idle = true;
}

bool AsyncASBuilder::IsIdle() const {
    return _idle;
}

auto AsyncASBuilder::GetUploadFinishedFence() const -> const Fence & {
    return _uploadFinishedFence;
}

void AsyncASBuilder::ConditionalGrowInstanceBuffer(size_t instanceCount) {
    size_t bufferSize = AlignUp(instanceCount * sizeof(D3D12_RAYTRACING_INSTANCE_DESC),
        D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    if (_pInstanceBuffer == nullptr || _pInstanceBuffer->GetSize() < bufferSize) {
        D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
        D3D12MA::Allocator *pAllocator = _pDevice->GetAllocator();
        D3D12MA::ALLOCATION_DESC allocationDesc = {};
        allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
        ThrowIfFailed(pAllocator->CreateResource(&allocationDesc,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            &_pInstanceBuffer,
            IID_NULL,
            nullptr));
        _pInstanceBuffer->GetResource()->SetName(L"AsyncASBuilder::InstanceBuffer");
    }
}

void AsyncASBuilder::ConditionalGrowScratchBuffer(size_t scratchBufferSize) {
    if (_pScratchBuffer == nullptr || _pScratchBuffer->GetSize() < scratchBufferSize) {
        D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(scratchBufferSize,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        D3D12MA::Allocator *pAllocator = _pDevice->GetAllocator();
        D3D12MA::ALLOCATION_DESC allocationDesc = {};
        allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
        ThrowIfFailed(pAllocator->CreateResource(&allocationDesc,
            &bufferDesc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            nullptr,
            &_pScratchBuffer,
            IID_NULL,
            nullptr));

        _pScratchBuffer->GetResource()->SetName(L"AsyncASBuilder::ScratchBuffer");
    }
}


}    // namespace dx
