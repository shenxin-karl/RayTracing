// © 2021 NVIDIA Corporation

#include "SharedD3D11.h"
#include "CommandQueueD3D11.h"

using namespace nri;

//================================================================================================================
// NRI
//================================================================================================================

inline void CommandQueueD3D11::Submit(const QueueSubmitDesc& queueSubmitDesc)
{
    for (uint32_t i = 0; i < queueSubmitDesc.commandBufferNum; i++)
    {
        CommandBufferHelper* commandBuffer = (CommandBufferHelper*)queueSubmitDesc.commandBuffers[i];
        commandBuffer->Submit();
    }
}

inline Result CommandQueueD3D11::ChangeResourceStates(const TransitionBarrierDesc& transitionBarriers)
{
    HelperResourceStateChange resourceStateChange(m_Device.GetCoreInterface(), (Device&)m_Device, (CommandQueue&)*this);

    return resourceStateChange.ChangeStates(transitionBarriers);
}

inline Result CommandQueueD3D11::UploadData(const TextureUploadDesc* textureUploadDescs, uint32_t textureUploadDescNum,
    const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum)
{
    HelperDataUpload helperDataUpload(m_Device.GetCoreInterface(), (Device&)m_Device, m_Device.GetStdAllocator(), (CommandQueue&)*this);

    return helperDataUpload.UploadData(textureUploadDescs, textureUploadDescNum, bufferUploadDescs, bufferUploadDescNum);
}

inline Result CommandQueueD3D11::WaitForIdle()
{
    HelperWaitIdle helperWaitIdle(m_Device.GetCoreInterface(), (Device&)m_Device, (CommandQueue&)*this);

    return helperWaitIdle.WaitIdle();
}

#include "CommandQueueD3D11.hpp"
