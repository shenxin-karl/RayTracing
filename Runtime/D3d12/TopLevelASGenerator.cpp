#include "TopLevelASGenerator.h"
#include "ASBuilder.h"
#include "Device.h"
#include "Buffer.h"
#include "Context.h"

namespace dx {

TopLevelASGenerator::TopLevelASGenerator() {
}

void TopLevelASGenerator::AddInstance(const ASInstance &instance) {
    D3D12_RAYTRACING_INSTANCE_DESC desc;
    glm::mat4x4 matrix = glm::transpose(instance.transform);
    std::memcpy(desc.Transform, glm::value_ptr(matrix), sizeof(desc.Transform));
    desc.InstanceID = instance.instanceID;
    desc.InstanceMask = instance.instanceMask;
    desc.InstanceContributionToHitGroupIndex = instance.hitGroupIndex;
    desc.Flags = static_cast<uint8_t>(instance.instanceFlag);
    desc.AccelerationStructure = instance.pBottomLevelAs->GetGPUVirtualAddress();
    _instances.push_back(desc);
}

void TopLevelASGenerator::AddInstance(ID3D12Resource *pBottomLevelAs,
    const glm::mat4x4 &transform,
    uint32_t instanceID,
    uint32_t hitGroupIndex,
    uint16_t instanceMask) {
    AddInstance(ASInstance{pBottomLevelAs, transform, instanceID, hitGroupIndex, instanceMask});
}

auto TopLevelASGenerator::CommitBuildCommand(IASBuilder *pASBuilder, TopLevelAS *pPreviousResult)
    -> SharedPtr<TopLevelAS> {

    SharedPtr<TopLevelAS> pResult;
#if ENABLE_RAY_TRACING
    if (pPreviousResult != nullptr && pPreviousResult->GetInstanceCount() != _instances.size()) {
        const char *pMsg = "Cannot be updated on the existing acceleration structure because the number of instances "
                           "has changed";
        Exception::Throw(pMsg);
    }

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS preBuildDesc = {};
    preBuildDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    preBuildDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    preBuildDesc.NumDescs = static_cast<UINT>(_instances.size());
    preBuildDesc.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    if (pPreviousResult != nullptr) {
        preBuildDesc.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
    }

    NativeDevice *device = pASBuilder->GetDevice()->GetNativeDevice();
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
    device->GetRaytracingAccelerationStructurePrebuildInfo(&preBuildDesc, &info);
    Assert(info.ResultDataMaxSizeInBytes > 0);
    info.ResultDataMaxSizeInBytes = AlignUp(info.ResultDataMaxSizeInBytes,
        D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    pResult = TopLevelAS::Create(pASBuilder->GetDevice(), info.ResultDataMaxSizeInBytes);
    pResult->_instanceCount = _instances.size();
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
    buildDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    buildDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    buildDesc.Inputs.InstanceDescs = 0;
    buildDesc.Inputs.NumDescs = static_cast<UINT>(_instances.size());
    buildDesc.DestAccelerationStructureData = pResult->GetGPUVirtualAddress();
    buildDesc.ScratchAccelerationStructureData = 0;
    if (pPreviousResult != nullptr) {
        buildDesc.SourceAccelerationStructureData = pPreviousResult->GetGPUVirtualAddress();
        buildDesc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
        pASBuilder->TraceResource(pPreviousResult->GetAllocation());
    }

    SyncASBuilder::TopASBuildItem buildItem;
    buildItem.desc = buildDesc;
    buildItem.scratchBufferSize = info.ScratchDataSizeInBytes;
    buildItem.instances = std::move(_instances);
    buildItem.pOutputResource = pResult->GetResource();
    pASBuilder->AddBuildItem(std::move(buildItem));
#endif
    return pResult;
}

auto TopLevelASGenerator::Build(const BuildArgs &buildArgs) -> SharedPtr<TopLevelAS> {
    SharedPtr<TopLevelAS> pResult = nullptr;
#if ENABLE_RAY_TRACING
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS preBuildDesc = {};
    preBuildDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    preBuildDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    preBuildDesc.NumDescs = static_cast<UINT>(_instances.size());
    preBuildDesc.Flags = buildArgs.flags;

    NativeDevice *device = buildArgs.pDevice->GetNativeDevice();
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
    device->GetRaytracingAccelerationStructurePrebuildInfo(&preBuildDesc, &info);
    Assert(info.ResultDataMaxSizeInBytes > 0);
    info.ResultDataMaxSizeInBytes = AlignUp(info.ResultDataMaxSizeInBytes,
        D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    #if MODE_DEBUG
    if (buildArgs.pInstanceBuffer != nullptr) {
        Assert(buildArgs.pInstanceBuffer->IsDynamicBuffer());
    }
    if (buildArgs.pScratchBuffer != nullptr) {
        Assert(buildArgs.pScratchBuffer->IsStaticBuffer());
    }
    #endif

    size_t instanceBufferSize = _instances.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
    size_t scratchBufferSize = info.ScratchDataSizeInBytes;

    instanceBufferSize = AlignUp(instanceBufferSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    scratchBufferSize = AlignUp(scratchBufferSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    if (buildArgs.pInstanceBuffer == nullptr || buildArgs.pInstanceBuffer->GetBufferSize() < instanceBufferSize) {
        buildArgs.pInstanceBuffer = Buffer::CreateDynamic(buildArgs.pDevice, instanceBufferSize);
    }

    if (buildArgs.pScratchBuffer == nullptr || buildArgs.pScratchBuffer->GetBufferSize() < scratchBufferSize) {
        buildArgs.pScratchBuffer = Buffer::CreateStatic(buildArgs.pDevice, scratchBufferSize);
    }

    pResult = TopLevelAS::Create(buildArgs.pDevice, info.ResultDataMaxSizeInBytes);
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
    buildDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    buildDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    buildDesc.Inputs.NumDescs = static_cast<UINT>(_instances.size());
    buildDesc.DestAccelerationStructureData = pResult->GetGPUVirtualAddress();
    buildDesc.ScratchAccelerationStructureData = buildArgs.pScratchBuffer->GetResource()->GetGPUVirtualAddress();
    buildDesc.Inputs.InstanceDescs = buildArgs.pInstanceBuffer->GetResource()->GetGPUVirtualAddress();

    D3D12_RAYTRACING_INSTANCE_DESC *pInstanceBuffer = nullptr;
    buildArgs.pInstanceBuffer->GetResource()->Map(0, nullptr, reinterpret_cast<void **>(&pInstanceBuffer));
    std::memcpy(pInstanceBuffer, _instances.data(), _instances.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
    buildArgs.pInstanceBuffer->GetResource()->Unmap(0, nullptr);
    _instances.clear();

    NativeCommandList *pCommandList = buildArgs.pComputeContext->GetCommandList();
    pCommandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);
    pCommandList->ResourceBarrier(1, RVPtr(CD3DX12_RESOURCE_BARRIER::UAV(pResult->GetResource())));
#endif
    return pResult;
}

}    // namespace dx
