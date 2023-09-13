#include "Texture.h"
#include "Device.h"
#include "ResourceStateTracker.h"
#include "Foundation/StringUtil.h"

namespace dx {

Texture::Texture() : _pResource(nullptr), _textureDesc{} {
    _pDevice = nullptr;
    _pAllocation = nullptr;
}

Texture::~Texture() {
    OnDestroy();
}

void Texture::OnCreate(Device *pDevice,
    const D3D12_RESOURCE_DESC &desc,
    D3D12_RESOURCE_STATES initState,
    const D3D12_CLEAR_VALUE *pClearValue) {

    _pDevice = pDevice;
    _textureDesc = desc;

    D3D12MA::Allocator *pAllocator = pDevice->GetAllocator();
    D3D12MA::ALLOCATION_DESC allocationDesc = {};
    allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

    // clang-format off
    ThrowIfFailed(pAllocator->CreateResource(&allocationDesc, 
        &desc, 
        initState, 
        pClearValue, 
        &_pAllocation, 
        IID_PPV_ARGS(&_pResource)));
    // clang-format on
    GlobalResourceState::SetResourceState(_pResource.Get(), initState);
}

void Texture::OnDestroy() {
    _pResource = nullptr;
    if (_pAllocation != nullptr) {
        _pAllocation->Release();
        _pAllocation = nullptr;
    }
    GlobalResourceState::RemoveResourceState(_pResource.Get());

}

void Texture::SetName(std::string_view name) {
    std::wstring wideName = nstd::to_wstring(name);
    _pResource->SetName(wideName.c_str());
}

}    // namespace dx
