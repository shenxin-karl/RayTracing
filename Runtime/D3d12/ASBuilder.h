#pragma once
#include "D3dStd.h"
#include "Fence.h"
#include "TopLevelASGenerator.h"

namespace dx {

class IASBuilder : private NonCopyable {
public:
    struct BottomASBuildItem {
        size_t scratchBufferSize;
        ID3D12Resource *pOutputResource;
        std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> vertexBuffers;
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags;
    };
    struct TopASBuildItem {
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc;
        size_t scratchBufferSize;
        std::vector<ASInstance> instances;
        ID3D12Resource *pOutputResource;
    };

    friend class TopLevelASGenerator;
    friend class BottomLevelASGenerator;
    virtual void AddBuildItem(BottomASBuildItem buildItem) = 0;
    virtual void AddBuildItem(TopASBuildItem buildItem) = 0;
    virtual auto GetDevice() const -> Device * = 0;
    // When updating the top-level acceleration structure, 
    // we need to keep track of old resources and release them when the build is complete
    virtual void TraceResource(WRL::ComPtr<D3D12MA::Allocation> pResource) = 0;
    virtual ~IASBuilder() = default;
};

class AsyncASBuilder : public virtual IASBuilder {
public:
    AsyncASBuilder();
    ~AsyncASBuilder() override;
    virtual void OnCreate(Device *pDevice);
    virtual void OnDestroy();
public:
    void Flush();
    void Reset();
    bool IsIdle() const;
    auto GetUploadFinishedFence() const -> const Fence &;
    auto GetBuildCommandCount() const -> size_t {
	    return _topAsBuildItems.size() + _bottomAsBuildItems.size();
    }
	auto GetDevice() const -> Device * override {
		return _pDevice;
	}
protected:
    void AddBuildItem(TopASBuildItem topAsBuildItem) override {
        Assert(IsIdle());
	    _topAsBuildItems.push_back(std::move(topAsBuildItem));
    }
    void AddBuildItem(BottomASBuildItem bottomAsBuildItem) override {
        Assert(IsIdle());
	    _bottomAsBuildItems.push_back(std::move(bottomAsBuildItem));
    }
    void TraceResource(WRL::ComPtr<D3D12MA::Allocation> pResource) override {
	    _traceResourceList.push_back(pResource);
    }
private:
    void ConditionalGrowInstanceBuffer(size_t instanceCount);
    void ConditionalGrowScratchBuffer(size_t scratchBufferSize);
    using TraceResourceList = std::vector<WRL::ComPtr<D3D12MA::Allocation>>;
protected:
    Device                             *_pDevice            = nullptr;
    WRL::ComPtr<NativeCommandList>      _pCommandList       = nullptr;
    WRL::ComPtr<ID3D12CommandAllocator> _pCommandAllocator  = nullptr;
    WRL::ComPtr<D3D12MA::Allocation>    _pScratchBuffer     = nullptr;
    WRL::ComPtr<D3D12MA::Allocation>    _pInstanceBuffer    = nullptr;
    std::vector<BottomASBuildItem>      _bottomAsBuildItems;
    std::vector<TopASBuildItem>         _topAsBuildItems;
    TraceResourceList                   _traceResourceList;
    bool                                _idle;
    Fence                               _uploadFinishedFence;
};

class SyncASBuilder : private AsyncASBuilder, public virtual IASBuilder {
public:
    using AsyncASBuilder::OnCreate;
    using AsyncASBuilder::OnDestroy;
    using AsyncASBuilder::GetDevice;
    using AsyncASBuilder::TraceResource;

    void FlushAndFinish() {
        if (GetBuildCommandCount() > 0) {
		    Flush();
	        _uploadFinishedFence.CpuWaitForFence();
	        Reset();
        }
    }
    void SetMaxBuildItem(size_t maxBuildItem) {
	    _maxBuildItem = maxBuildItem;
    }
protected:
    void AddBuildItem(BottomASBuildItem buildItem) override {
	    AsyncASBuilder::AddBuildItem(std::move(buildItem));
        ConditionalFlushAndFinish();
    }
    void AddBuildItem(TopASBuildItem buildItem) override {
	    AsyncASBuilder::AddBuildItem(std::move(buildItem));
        ConditionalFlushAndFinish();
    }
private:
    void ConditionalFlushAndFinish() {
	    if ((_bottomAsBuildItems.size() + _topAsBuildItems.size()) >= _maxBuildItem) {
			FlushAndFinish();
		}
    }
private:
    size_t  _maxBuildItem = 20;
};

}    // namespace dx
