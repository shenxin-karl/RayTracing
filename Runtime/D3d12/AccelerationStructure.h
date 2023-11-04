#pragma once
#include "D3dUtils.h"

namespace dx {

class AccelerationStructure : public NonCopyable {
public:
    AccelerationStructure();
    ~AccelerationStructure();
    AccelerationStructure(AccelerationStructure &&other) noexcept;
    AccelerationStructure &operator=(AccelerationStructure &&other) noexcept;

    void OnCreate(Device *pDevice, size_t bufferSize);
    void OnDestroy();
    void SetName(std::string_view name);

    auto GetResource() const -> ID3D12Resource * {
        return _pAllocation != nullptr ? _pAllocation->GetResource() : nullptr;
    }
    auto GetGPUVirtualAddress() const -> D3D12_GPU_VIRTUAL_ADDRESS {
        return GetResource() != nullptr ? GetResource()->GetGPUVirtualAddress() : 0;
    }
    bool IsNull() const {
        return _pAllocation != nullptr;
    }
protected:
    WRL::ComPtr<D3D12MA::Allocation> _pAllocation;
};

class BottomLevelAS : public AccelerationStructure {};

class TopLevelAS : public AccelerationStructure {
public:
    auto GetInstanceCount() const -> size_t {
        return _instanceCount;
    }
private:
    friend class TopLevelASGenerator;
    size_t _instanceCount = 0;
};

}    // namespace dx
