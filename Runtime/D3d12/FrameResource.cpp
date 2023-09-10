#include "FrameResource.h"
#include "CommandListPool.h"
#include "Context.h"
#include "Device.h"

namespace dx {

FrameResource::FrameResource() : _pDevice(nullptr), _fenceValue(0) {
}

FrameResource::~FrameResource() {
}

void FrameResource::OnCreate(Device *pDevice, uint32_t numGraphicsCmdListPreFrame, uint32_t numComputeCmdListPreFrame) {
    _pDevice = pDevice;
    _pGraphicsCmdListPool = std::make_unique<CommandListPool>();
    _pComputeCmdListPool = std::make_unique<CommandListPool>();
    _pGraphicsCmdListPool->OnCreate(pDevice, numGraphicsCmdListPreFrame, D3D12_COMMAND_LIST_TYPE_DIRECT);
    _pComputeCmdListPool->OnCreate(pDevice, numComputeCmdListPreFrame, D3D12_COMMAND_LIST_TYPE_COMPUTE);
}

void FrameResource::OnDestroy() {
    _pGraphicsCmdListPool->OnDestroy();
    _pComputeCmdListPool->OnDestroy();
    _pGraphicsCmdListPool = nullptr;
    _pComputeCmdListPool = nullptr;
}

void FrameResource::OnBeginFrame(uint64_t newFenceValue) {
    _fenceValue = newFenceValue;
    _pGraphicsCmdListPool->OnBeginFrame();
    _pGraphicsCmdListPool->OnBeginFrame();
}

auto FrameResource::AllocGraphicsContext() -> std::shared_ptr<GraphicsContext> {
    return std::make_shared<GraphicsContext>(_pGraphicsCmdListPool->AllocCommandList());
}

auto FrameResource::AllocComputeContext() -> std::shared_ptr<ComputeContext> {
    return std::make_shared<ComputeContext>(_pComputeCmdListPool->AllocCommandList());
}

void FrameResource::ExecuteContexts(ReadonlyArraySpan<Context *> contexts) {
	for (int i = 0; i < static_cast<int>(contexts.Count()) - 1; ++i) {
		Context *pPrevContext = contexts[i-1];
        Context *pCurrContext = contexts[i];
        pCurrContext->_resourceStateTracker.FlushPendingResourceBarriers(pPrevContext->GetCommandList());
	}

    for (Context *pContext : contexts) {
	    ThrowIfFailed(pContext->GetCommandList()->Close());
    }

    std::vector<ID3D12CommandList *> graphicsCmdList;
    std::vector<ID3D12CommandList *> computeCmdList;
    for (Context *pContext : contexts) {
        if (pContext->GetContextType() == ContextType::eGraphics) {
			graphicsCmdList.push_back(pContext->GetCommandList());
        } else {
	        computeCmdList.push_back(pContext->GetCommandList());
        }
    }

    _pDevice->GetGraphicsQueue()->ExecuteCommandLists(graphicsCmdList.size(), graphicsCmdList.data());
    _pDevice->GetComputeQueue()->ExecuteCommandLists(computeCmdList.size(), computeCmdList.data());
}

}    // namespace dx
