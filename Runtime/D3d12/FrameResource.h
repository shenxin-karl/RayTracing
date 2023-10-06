#pragma once
#include "D3dUtils.h"
#include "Foundation/ReadonlyArraySpan.hpp"

namespace dx {

class FrameResource : NonCopyable {
public:
    friend class FrameResourceRing;
    FrameResource();
    ~FrameResource();
private:
    void OnCreate(Device *pDevice, uint32_t numGraphicsCmdListPreFrame, uint32_t numComputeCmdListPreFrame);
    void OnDestroy();
    void OnBeginFrame(uint64_t newFenceValue);
    auto GetFenceValue() const -> uint64_t {
        return _fenceValue;
    }
public:
    auto AllocGraphicsContext() -> std::shared_ptr<GraphicsContext>;
#if ENABLE_D3D_COMPUTE_QUEUE
    auto AllocComputeContext() -> std::shared_ptr<ComputeContext>;
#endif
    void ExecuteContexts(ReadonlyArraySpan<Context *> contexts);
private:
    using GraphicsContextList = std::vector<std::shared_ptr<GraphicsContext>>;
    using ComputeContextList = std::vector<std::shared_ptr<ComputeContext>>;
private:
    // clang-format off
    Device                          *_pDevice;
    uint64_t                         _fenceValue;
    std::unique_ptr<CommandListPool> _pGraphicsCmdListPool;
    GraphicsContextList              _graphicsContextList;
    size_t                           _graphicsContextIndex;
#if ENABLE_D3D_COMPUTE_QUEUE
    std::unique_ptr<CommandListPool> _pComputeCmdListPool;
    ComputeContextList               _computeContextList;
    size_t                           _computeContextIndex;
#endif

    // clang-format on
};

}    // namespace dx
