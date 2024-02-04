#pragma once
#include "D3dStd.h"
#include "Foundation/Memory/SharedPtr.hpp"

namespace dx {

class AccelerationStructure : public RefCounter {
protected:
    AccelerationStructure(Device *pDevice, size_t bufferSize);
public:
    ~AccelerationStructure() override;
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
private:
    friend class TopLevelASGenerator;
    auto GetAllocation() const -> WRL::ComPtr<D3D12MA::Allocation> {
        return _pAllocation;
    }
protected:
    WRL::ComPtr<D3D12MA::Allocation> _pAllocation;
};

class BottomLevelAS : public AccelerationStructure {
protected:
    using AccelerationStructure::AccelerationStructure;
public:
    template<typename... Args>
    static SharedPtr<BottomLevelAS> Create(Device *pDevice, Args &&...args) {
        return SharedPtr<BottomLevelAS>(new BottomLevelAS(pDevice, std::forward<Args>(args)...));
    }
};

class TopLevelAS : public AccelerationStructure {
protected:
    using AccelerationStructure::AccelerationStructure;
public:
    template<typename... Args>
    static SharedPtr<TopLevelAS> Create(Device *pDevice, Args &&...args) {
        return SharedPtr<TopLevelAS>(new TopLevelAS(pDevice, std::forward<Args>(args)...));
    }
    auto GetInstanceCount() const -> size_t {
	    return _instanceCount;
    }
private:
    friend TopLevelASGenerator;
    size_t _instanceCount = 0;
};

}    // namespace dx
