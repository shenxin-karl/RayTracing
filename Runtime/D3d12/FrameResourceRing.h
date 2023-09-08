#pragma once
#include "D3dUtils.h"

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
private :
	using FrameResourcePool = std::vector<std::unique_ptr<FrameResource>>;
private:
    // clang-format off
    Device              *_pDevice = nullptr;
    FrameResourcePool    _frameResourcePool;
    // clang-format on
};

}    // namespace dx
