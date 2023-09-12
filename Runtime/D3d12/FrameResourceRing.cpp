#include "FrameResourceRing.h"
#include "Device.h"
#include "FrameResource.h"

namespace dx {

FrameResourceRing::FrameResourceRing() : _pDevice(nullptr), _frameIndex(0) {
}

FrameResourceRing::~FrameResourceRing() {
}

void FrameResourceRing::OnCreate(const FrameResourceRingDesc &desc) {
	for (size_t i = 0; i < desc.numFrameResource; ++i) {
		_frameResourcePool.push_back(std::make_unique<FrameResource>());
		_frameResourcePool[i]->OnCreate(desc.pDevice, desc.numGraphicsCmdListPreFrame, desc.numComputeCmdListPreFrame);
	}

	_graphicsQueueFence.OnCreate(_pDevice, "FrameResourceRing::_graphicsQueueFence");
	_computeQueueFence.OnCreate(_pDevice, "FrameResourceRing::_computeQueueFence");
}

void FrameResourceRing::OnDestroy() {
	for (std::unique_ptr<FrameResource> &pFrameResource : _frameResourcePool) {
		pFrameResource->OnDestroy();
	}
	_frameResourcePool.clear();
	_graphicsQueueFence.OnDestroy();
	_computeQueueFence.OnDestroy();
}

void FrameResourceRing::OnBeginFrame() {
	++_frameIndex;
	size_t index = _frameIndex % _frameResourcePool.size();
	uint64_t fenceValue = _frameResourcePool[index]->GetFenceValue();
	_computeQueueFence.CpuWaitForFence(fenceValue);
	_graphicsQueueFence.CpuWaitForFence(fenceValue);
	 _frameResourcePool[index]->OnBeginFrame(_frameIndex);
}

void FrameResourceRing::OnEndFrame() {
	_graphicsQueueFence.IssueFence(_pDevice->GetGraphicsQueue());
#if ENABLE_D3D_COMPUTE_QUEUE
	_computeQueueFence.IssueFence(_pDevice->GetComputeQueue());
#endif
}

auto FrameResourceRing::GetCurrentFrameResource() const -> FrameResource & {
	size_t index = _frameIndex % _frameResourcePool.size();
	return *_frameResourcePool[index];
}

}    // namespace dx
