#pragma once
#include "D3dUtils.h"
#include "Fence.h"

namespace dx {

// acceleration structure builder
class ASBuilder : public NonCopyable {
public:
    ~ASBuilder();
    void OnCreate(Device *pDevice);
    void OnDestroy();

    void BeginBuild();
    void EndBuild();

    void BuildBottomAS(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC &desc,
        size_t scratchBufferSize,
        ID3D12Resource *pOutputResource);
    void BuildTopAS(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC &desc,
        size_t scratchBufferSize,
        std::vector<ASInstance> instances,
        ID3D12Resource *pOutputResource);

    auto GetDevice() const -> Device * {
        return _pDevice;
    }
    auto GetBuildFinishedFence() -> Fence & {
        return _buildFinishedFence;
    }
private:
    void ConditionalGrowInstanceBuffer(size_t instanceCount);
    void ConditionalGrowScratchBuffer(size_t scratchBufferSize);
private:
    struct BottomASBuildItem {
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc;
        size_t scratchBufferSize;
        ID3D12Resource *pResource;
    };
    struct TopASBuildItem {
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc;
        size_t scratchBufferSize;
        std::vector<ASInstance> instances;
        ID3D12Resource *pResource;
    };

    // clang-format off
    Device                             *_pDevice            = nullptr;
    WRL::ComPtr<NativeCommandList>      _pCommandList       = nullptr;
    WRL::ComPtr<ID3D12CommandAllocator> _pCommandAllocator  = nullptr;
    WRL::ComPtr<D3D12MA::Allocation>    _pScratchBuffer     = nullptr;
    WRL::ComPtr<D3D12MA::Allocation>    _pInstanceBuffer    = nullptr;
    Fence                               _buildFinishedFence = {};
    std::vector<BottomASBuildItem>      _bottomAsBuildItems;
    std::vector<TopASBuildItem>         _topAsBuildItems;
    // clang-format on
};

}    // namespace dx
