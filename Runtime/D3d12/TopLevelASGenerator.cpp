#include "TopLevelASGenerator.h"
#include "ASBuilder.h"
#include "Device.h"
#include <glm/gtc/type_ptr.inl>

namespace dx {

TopLevelASGenerator::TopLevelASGenerator() {
}

void TopLevelASGenerator::AddInstance(const ASInstance &instance) {
    _instances.push_back(instance);
}

void TopLevelASGenerator::AddInstance(ID3D12Resource *pBottomLevelAs,
    const glm::mat4x4 &transform,
    uint32_t instanceID,
    uint32_t hitGroupIndex,
    uint16_t instanceMask) {
    AddInstance(ASInstance{pBottomLevelAs, transform, instanceID, hitGroupIndex, instanceMask});
}

auto TopLevelASGenerator::CommitBuildCommand(IASBuilder *pASBuilder, TopLevelAS *pPreviousResult)
    -> std::shared_ptr<TopLevelAS> {

    std::shared_ptr<TopLevelAS> pResult;
#if ENABLE_RAY_TRACING
    if (pPreviousResult != nullptr && pPreviousResult->GetInstanceCount() != _instances.size()) {
        const char *pMsg = "Cannot be updated on the existing acceleration structure because the number of instances "
                           "has changed";
        Exception::Throw(pMsg);
    }

    pResult = std::make_shared<TopLevelAS>();
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

    pResult->OnCreate(pASBuilder->GetDevice(), info.ResultDataMaxSizeInBytes);
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


}    // namespace dx
