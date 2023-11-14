#include "ASBuilder.h"
#include "Device.h"
#include <glm/gtc/type_ptr.inl>

#include "AccelerationStructure.h"

namespace dx {

ASBuilder::~ASBuilder() {
    OnDestroy();
}

void ASBuilder::OnCreate(Device *pDevice) {
    _pDevice = pDevice;
    ID3D12Device *device = pDevice->GetNativeDevice();

    device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_pCommandAllocator));
    device->CreateCommandList(0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        _pCommandAllocator.Get(),
        nullptr,
        IID_PPV_ARGS(&_pCommandList));

    _pCommandAllocator->SetName(L"ASBuilder::CommandListAllocator");
    _pCommandList->SetName(L"ASBuilder::CommandList");
    _buildFinishedFence.OnCreate(_pDevice, "ASBuilder::BuildFinishedFence");

    ThrowIfFailed(_pCommandList->Close());
    ID3D12CommandList *cmdList[] = {_pCommandList.Get()};
    _pDevice->GetCopyQueue()->ExecuteCommandLists(1, cmdList);
    _pDevice->WaitForGPUFlush(D3D12_COMMAND_LIST_TYPE_COPY);
}

void ASBuilder::OnDestroy() {
    _pDevice = nullptr;
    _pScratchBuffer = nullptr;
    _pCommandList = nullptr;
    _pCommandAllocator = nullptr;
    _pInstanceBuffer = nullptr;
    _buildFinishedFence.OnDestroy();
}

void ASBuilder::BeginBuild() {
    _bottomAsBuildItems.clear();
    _topAsBuildItems.clear();
    ThrowIfFailed(_pCommandAllocator->Reset());
    ThrowIfFailed(_pCommandList->Reset(_pCommandAllocator.Get(), nullptr));
}

void ASBuilder::EndBuild() {
    size_t instanceCount = 0;
    size_t scratchBufferSize = 0;
    for (BottomASBuildItem &bottomBuildItem : _bottomAsBuildItems) {
        scratchBufferSize = std::max(scratchBufferSize, bottomBuildItem.scratchBufferSize);
    }

    for (TopASBuildItem &topBuildItem : _topAsBuildItems) {
        scratchBufferSize = std::max(scratchBufferSize, topBuildItem.scratchBufferSize);
        instanceCount += topBuildItem.instances.size();
    }

    ConditionalGrowInstanceBuffer(instanceCount + 1);
    ConditionalGrowScratchBuffer(scratchBufferSize);

    D3D12_GPU_VIRTUAL_ADDRESS scratchBufferAddress = _pScratchBuffer->GetResource()->GetGPUVirtualAddress();
    for (BottomASBuildItem &bottomBuildItem : _bottomAsBuildItems) {
        bottomBuildItem.desc.ScratchAccelerationStructureData = scratchBufferAddress;
        _pCommandList->BuildRaytracingAccelerationStructure(&bottomBuildItem.desc, 0, nullptr);
        _pCommandList->ResourceBarrier(1, RVPtr(CD3DX12_RESOURCE_BARRIER::UAV(bottomBuildItem.pResource)));
    }
    _bottomAsBuildItems.clear();

    D3D12_RAYTRACING_INSTANCE_DESC *pBuffer = nullptr;
    _pInstanceBuffer->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&pBuffer));
    D3D12_GPU_VIRTUAL_ADDRESS instanceBufferAddress = _pInstanceBuffer->GetResource()->GetGPUVirtualAddress();

    for (TopASBuildItem &topBuildItem : _topAsBuildItems) {
		D3D12_RAYTRACING_INSTANCE_DESC* pInstance = pBuffer;
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
        _pCommandList->ResourceBarrier(1, RVPtr(CD3DX12_RESOURCE_BARRIER::UAV(topBuildItem.pResource)));
    }

    _topAsBuildItems.clear();
    _pInstanceBuffer->GetResource()->Unmap(0, nullptr);


    ThrowIfFailed(_pCommandList->Close());
    ID3D12CommandList *cmdList[] = {_pCommandList.Get()};
    _pDevice->GetCopyQueue()->ExecuteCommandLists(1, cmdList);
    _buildFinishedFence.IssueFence(_pDevice->GetCopyQueue());
}

void ASBuilder::BuildBottomAS(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC &desc,
    size_t scratchBufferSize,
    ID3D12Resource *pOutputResource) {
    _bottomAsBuildItems.push_back({desc, scratchBufferSize, pOutputResource});
}

void ASBuilder::BuildTopAS(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC &desc,
    size_t scratchBufferSize,
    std::vector<ASInstance> instances,
    ID3D12Resource *pOutputResource) {
    _topAsBuildItems.push_back({desc, scratchBufferSize, std::move(instances), pOutputResource});
}

void ASBuilder::ConditionalGrowInstanceBuffer(size_t instanceCount) {
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
        _pInstanceBuffer->GetResource()->SetName(L"ASBuilder::InstanceBuffer");
    }
}

void ASBuilder::ConditionalGrowScratchBuffer(size_t scratchBufferSize) {
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

        _pScratchBuffer->GetResource()->SetName(L"ASBuilder::ScratchBuffer");
    }
}

}    // namespace dx
