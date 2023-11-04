#include "TopLevelASGenerator.h"
#include "ASBuilder.h"
#include "Device.h"
#include <glm/gtc/type_ptr.inl>

namespace dx {

TopLevelASGenerator::TopLevelASGenerator() : _allowUpdate(false), _scratchSizeInBytes(0), _resultSizeInBytes(0) {
}

void TopLevelASGenerator::AddInstance(const Instance &instance) {
    _allowUpdate = false;
    _instances.push_back(instance);
}

void TopLevelASGenerator::AddInstance(BottomLevelAS *pBottomLevelAs,
    const glm::mat3x4 &transform,
    uint32_t instanceID,
    uint32_t hitGroupIndex) {

    AddInstance(Instance{pBottomLevelAs, transform, instanceID, hitGroupIndex});
}

void TopLevelASGenerator::ComputeAsBufferSizes(ASBuilder *pUploadHeap, bool allowUpdate) {
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS preBuildDesc = {};
    preBuildDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    preBuildDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    preBuildDesc.NumDescs = static_cast<UINT>(_instances.size());
    preBuildDesc.Flags = allowUpdate ? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE
                                     : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

    NativeDevice *device = pUploadHeap->GetDevice()->GetNativeDevice();
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
    device->GetRaytracingAccelerationStructurePrebuildInfo(&preBuildDesc, &info);

    _resultSizeInBytes = AlignUp(info.ResultDataMaxSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    _scratchSizeInBytes = AlignUp(info.ScratchDataSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    pUploadHeap->UpdateInstanceBufferElementCount(_instances.size());
}

auto TopLevelASGenerator::Generate(ASBuilder *pUploadHeap, TopLevelAS *pPreviousResult) -> TopLevelAS {
    if (pPreviousResult != nullptr && !_allowUpdate) {
        const char *pMsg = "Cannot be updated on the existing acceleration structure because the number of instances "
                           "has changed";
        Exception::Throw(pMsg);
    }

    constexpr size_t kTransformSize = sizeof(D3D12_RAYTRACING_INSTANCE_DESC::Transform);
    std::span<D3D12_RAYTRACING_INSTANCE_DESC> instanceBuffer = pUploadHeap->GetInstanceBuffer();
    for (size_t i = 0; i < _instances.size(); ++i) {
        instanceBuffer[i].InstanceID = _instances[i].instanceID;
        instanceBuffer[i].InstanceContributionToHitGroupIndex = _instances[i].hitGroupIndex;
        instanceBuffer[i].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        instanceBuffer[i].AccelerationStructure = _instances[i].pBottomLevelAs->GetGPUVirtualAddress();
        std::memcpy(instanceBuffer[i].Transform, glm::value_ptr(_instances[i].transform), kTransformSize);
        // Visibility mask, always visible here
        instanceBuffer[i].InstanceMask = 0xFF;
    }

    TopLevelAS result;
    result.OnCreate(pUploadHeap->GetDevice(), _resultSizeInBytes);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
    buildDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    buildDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    buildDesc.Inputs.InstanceDescs = pUploadHeap->GetInstanceBufferGPUAddress();
    buildDesc.Inputs.NumDescs = static_cast<UINT>(_instances.size());
    buildDesc.DestAccelerationStructureData = result.GetGPUVirtualAddress();
    buildDesc.ScratchAccelerationStructureData = pUploadHeap->GetScratchBufferGPUAddress();
    if (pPreviousResult != nullptr) {
        buildDesc.SourceAccelerationStructureData = pPreviousResult->GetGPUVirtualAddress();
        buildDesc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
    }
    pUploadHeap->BuildRayTracingAccelerationStructure(buildDesc, result.GetResource());
    return result;
}

}    // namespace dx
