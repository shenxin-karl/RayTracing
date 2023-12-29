#include "TopLevelASGenerator.h"
#include "ASBuilder.h"
#include "Device.h"
#include <glm/gtc/type_ptr.inl>

namespace dx {

TopLevelASGenerator::TopLevelASGenerator() : _allowUpdate(false) {
}

void TopLevelASGenerator::AddInstance(const ASInstance &instance) {
    _allowUpdate = false;
    _instances.push_back(instance);
}

void TopLevelASGenerator::AddInstance(ID3D12Resource *pBottomLevelAs,
    const glm::mat4x4 &transform,
    uint32_t instanceID,
    uint32_t hitGroupIndex,
    uint16_t instanceMask) {
    AddInstance(ASInstance{pBottomLevelAs, transform, instanceID, hitGroupIndex, instanceMask});
}

auto TopLevelASGenerator::CommitCommand(ASBuilder *pUploadHeap, TopLevelAS *pPreviousResult, bool cleanUpInstances)
    -> TopLevelAS {

    TopLevelAS result;
#if ENABLE_RAY_TRACING
    if (pPreviousResult != nullptr && !_allowUpdate) {
        const char *pMsg = "Cannot be updated on the existing acceleration structure because the number of instances "
                           "has changed";
        Exception::Throw(pMsg);
    }

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS preBuildDesc = {};
    preBuildDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    preBuildDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    preBuildDesc.NumDescs = static_cast<UINT>(_instances.size());
    preBuildDesc.Flags = pPreviousResult != nullptr ? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE
                                                    : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

    NativeDevice *device = pUploadHeap->GetDevice()->GetNativeDevice();
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
    device->GetRaytracingAccelerationStructurePrebuildInfo(&preBuildDesc, &info);
    Assert(info.ResultDataMaxSizeInBytes > 0);
    info.ResultDataMaxSizeInBytes = AlignUp(info.ResultDataMaxSizeInBytes,
        D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    result.OnCreate(pUploadHeap->GetDevice(), info.ResultDataMaxSizeInBytes);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
    buildDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    buildDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    buildDesc.Inputs.InstanceDescs = 0;
    buildDesc.Inputs.NumDescs = static_cast<UINT>(_instances.size());
    buildDesc.DestAccelerationStructureData = result.GetGPUVirtualAddress();
    buildDesc.ScratchAccelerationStructureData = 0;
    if (pPreviousResult != nullptr) {
        buildDesc.SourceAccelerationStructureData = pPreviousResult->GetGPUVirtualAddress();
        buildDesc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
    }

    ASBuilder::TopASBuildItem buildItem;
    buildItem.desc = buildDesc;
    buildItem.scratchBufferSize = info.ScratchDataSizeInBytes;
    if (cleanUpInstances) {
		buildItem.instances =  std::move(_instances);
    } else {
        buildItem.instances = _instances;
		_allowUpdate = true;
    }

    buildItem.pResource = result.GetResource();
    pUploadHeap->AddBuildItem(std::move(buildItem));
#endif
    return result;
}

}    // namespace dx
