#pragma once
#include "D3dUtils.h"
#include "Fence.h"

namespace dx {

// todo: 需要修改为支持异步构建
// acceleration structure builder
class ASBuilder : private NonCopyable {
public:
    ~ASBuilder();
    void OnCreate(Device *pDevice, size_t maxBuildItem = 20);
    void OnDestroy();
    void FlushAndFinish();
    auto GetDevice() const -> Device * {
        return _pDevice;
    }
private:
    friend TopLevelASGenerator;
    friend BottomLevelASGenerator;

	struct BottomASBuildItem {
	    size_t scratchBufferSize;
	    ID3D12Resource *pResource;
	    std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> vertexBuffers;
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags;
	};
	struct TopASBuildItem {
	    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc;
	    size_t scratchBufferSize;
	    std::vector<ASInstance> instances;
	    ID3D12Resource *pResource;
	};

    void AddBuildItem(BottomASBuildItem &&buildItem) {
	    _bottomAsBuildItems.push_back(std::move(buildItem));
        ConditionalFlushAndFinish();
    }
    void AddBuildItem(TopASBuildItem &&buildItem) {
	    _topAsBuildItems.push_back(std::move(buildItem));
        ConditionalFlushAndFinish();
    }
private:
    void ConditionalGrowInstanceBuffer(size_t instanceCount);
    void ConditionalGrowScratchBuffer(size_t scratchBufferSize);
    void ConditionalFlushAndFinish();
private:
    // clang-format off
    size_t                              _maxBuildItem       = {};
    Device                             *_pDevice            = nullptr;
    WRL::ComPtr<NativeCommandList>      _pCommandList       = nullptr;
    WRL::ComPtr<ID3D12CommandAllocator> _pCommandAllocator  = nullptr;
    WRL::ComPtr<D3D12MA::Allocation>    _pScratchBuffer     = nullptr;
    WRL::ComPtr<D3D12MA::Allocation>    _pInstanceBuffer    = nullptr;
    std::vector<BottomASBuildItem>      _bottomAsBuildItems;
    std::vector<TopASBuildItem>         _topAsBuildItems;
    // clang-format on
};



}    // namespace dx
