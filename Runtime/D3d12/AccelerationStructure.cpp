#include "AccelerationStructure.h"
#include "Device.h"
#include "ResourceStateTracker.h"
#include "Foundation/StringUtil.h"

namespace dx {

AccelerationStructure::AccelerationStructure(Device *pDevice, size_t bufferSize) {
    Assert(_pAllocation == nullptr);

    D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    D3D12MA::Allocator *pAllocator = pDevice->GetAllocator();
    D3D12MA::ALLOCATION_DESC allocationDesc = {};
    allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

    ThrowIfFailed(pAllocator->CreateResource(&allocationDesc,
        &bufferDesc,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
        nullptr,
        &_pAllocation,
        IID_NULL,
        nullptr));

    GlobalResourceState::SetResourceState(_pAllocation->GetResource(),
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
}

AccelerationStructure::~AccelerationStructure() {
    if (_pAllocation != nullptr) {
        GlobalResourceState::RemoveResourceState(_pAllocation->GetResource());
        _pAllocation = nullptr;
    }
}

void AccelerationStructure::SetName(std::string_view name) {
    std::wstring wideName = nstd::to_wstring(name);
    GetResource()->SetName(wideName.c_str());
}

}    // namespace dx
