#pragma once
#include "D3dStd.h"
#include "Fence.h"

namespace dx {

struct FrameResourceRingDesc {
    Device *pDevice;
    uint32_t numFrameResource;
    uint32_t numGraphicsCmdListPreFrame;
    uint32_t numComputeCmdListPreFrame;
};

class FrameResourceRing : NonCopyable {
public:
    FrameResourceRing();
    ~FrameResourceRing();
public:
    void OnCreate(const FrameResourceRingDesc &desc);
    void OnDestroy();
    void OnBeginFrame();
    void OnEndFrame();
	auto GetCurrentFrameResource() const -> FrameResource &;
private :
	using FrameResourcePool = std::vector<std::unique_ptr<FrameResource>>;
private:
    // clang-format off
    Device              *_pDevice;
    uint32_t             _frameIndex;
    FrameResourcePool    _frameResourcePool;
    Fence                _graphicsQueueFence;
    Fence                _computeQueueFence;
    // clang-format on
};

}    // namespace dx
