#pragma once
#include "D3dUtils.h"

namespace dx {

class FrameResource : NonCopyable {
public:
	FrameResource();
	~FrameResource();
public:
	void OnCreate(Device *pDevice, uint32_t numGraphicsCmdListPreFrame, uint32_t numComputeCmdListPreFrame);
	void OnDestroy();
	void OnBeginFrame();
	void OnEndFrame(ID3D12CommandQueue *pGraphicsQueue, ID3D12CommandList *pComputeQueue);
private:
	uint64_t						 _fence;
	std::unique_ptr<CommandListPool> _pGraphicsCmdListPool;
	std::unique_ptr<CommandListPool> _pComputeCmdListPool;
};

}
