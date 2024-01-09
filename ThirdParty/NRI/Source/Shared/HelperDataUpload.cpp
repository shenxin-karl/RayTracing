#include "SharedExternal.h"

using namespace nri;

constexpr size_t BASE_UPLOAD_BUFFER_SIZE = 65536;

HelperDataUpload::HelperDataUpload(const CoreInterface& NRI, Device& device, const StdAllocator<uint8_t>& stdAllocator, CommandQueue& commandQueue) :
    NRI(NRI),
    m_Device(device),
    m_CommandQueue(commandQueue),
    m_CommandBuffers(stdAllocator),
    m_UploadBufferSize(BASE_UPLOAD_BUFFER_SIZE)
{
}

Result HelperDataUpload::UploadData(const TextureUploadDesc* textureUploadDescs, uint32_t textureUploadDescNum,
    const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum)
{
    const DeviceDesc& deviceDesc = NRI.GetDeviceDesc(m_Device);

    for (uint32_t i = 0; i < textureUploadDescNum; i++)
    {
        if (textureUploadDescs[i].subresources == nullptr)
            continue;

        const TextureSubresourceUploadDesc& subresource = textureUploadDescs[i].subresources[0];

        const uint32_t sliceRowNum = std::max(subresource.slicePitch / subresource.rowPitch, 1u);
        const uint64_t alignedRowPitch = Align(subresource.rowPitch, deviceDesc.uploadBufferTextureRowAlignment);
        const uint64_t alignedSlicePitch = Align(sliceRowNum * alignedRowPitch, deviceDesc.uploadBufferTextureSliceAlignment);
        const uint64_t mipLevelContentSize = alignedSlicePitch * std::max(subresource.sliceNum, 1u);
        m_UploadBufferSize = std::max(m_UploadBufferSize, mipLevelContentSize);
    }

    m_UploadBufferSize = Align(m_UploadBufferSize, COPY_ALIGMENT);

    Result result = Create();

    if (result != Result::SUCCESS)
        return result;

    result = UploadTextures(textureUploadDescs, textureUploadDescNum);

    if (result == Result::SUCCESS)
        result = UploadBuffers(bufferUploadDescs, bufferUploadDescNum);

    Destroy();

    return result;
}

Result HelperDataUpload::Create()
{
    BufferDesc bufferDesc = {};
    bufferDesc.size = m_UploadBufferSize;
    Result result = NRI.CreateBuffer(m_Device, bufferDesc, m_UploadBuffer);
    if (result != Result::SUCCESS)
        return result;

    MemoryDesc memoryDesc = {};
    NRI.GetBufferMemoryInfo(*m_UploadBuffer, MemoryLocation::HOST_UPLOAD, memoryDesc);

    result = NRI.AllocateMemory(m_Device, ALL_NODES, memoryDesc.type, memoryDesc.size, m_UploadBufferMemory);
    if (result != Result::SUCCESS)
        return result;

    const BufferMemoryBindingDesc bufferMemoryBindingDesc = { m_UploadBufferMemory, m_UploadBuffer, 0 };
    result = NRI.BindBufferMemory(m_Device, &bufferMemoryBindingDesc, 1);
    if (result != Result::SUCCESS)
        return result;

    result = NRI.CreateFence(m_Device, 0, m_Fence);
    if (result != Result::SUCCESS)
        return result;

    const DeviceDesc& deviceDesc = NRI.GetDeviceDesc(m_Device);
    m_CommandBuffers.resize(deviceDesc.nodeNum);

    result = NRI.CreateCommandAllocator(m_CommandQueue, m_CommandAllocators);
    if (result != Result::SUCCESS)
        return result;

    for (uint32_t i = 0; i < m_CommandBuffers.size(); i++)
    {
        result = NRI.CreateCommandBuffer(*m_CommandAllocators, m_CommandBuffers[i]);
        if (result != Result::SUCCESS)
            return result;
    }

    return result;
}

void HelperDataUpload::Destroy()
{
    for (uint32_t i = 0; i < m_CommandBuffers.size(); i++)
    {
        NRI.DestroyCommandBuffer(*m_CommandBuffers[i]);
        NRI.DestroyCommandAllocator(*m_CommandAllocators);
    }

    NRI.DestroyFence(*m_Fence);
    NRI.DestroyBuffer(*m_UploadBuffer);
    NRI.FreeMemory(*m_UploadBufferMemory);
}

Result HelperDataUpload::UploadTextures(const TextureUploadDesc* textureUploadDescs, uint32_t textureDataDescNum)
{
    Result result = DoTransition<true>(textureUploadDescs, textureDataDescNum);
    if (result != Result::SUCCESS)
        return result;

    uint32_t i = 0;
    Dim_t arrayOffset = 0;
    Mip_t mipOffset = 0;

    while (i < textureDataDescNum)
    {
        result = BeginCommandBuffers();
        if (result != Result::SUCCESS)
            return result;

        m_UploadBufferOffset = 0;
        bool isCapacityInsufficient = false;

        for (; i < textureDataDescNum && CopyTextureContent(textureUploadDescs[i], arrayOffset, mipOffset, isCapacityInsufficient); i++)
            ;

        if (isCapacityInsufficient)
            return Result::OUT_OF_MEMORY;

        result = EndCommandBuffersAndSubmit();
        if (result != Result::SUCCESS)
            return result;
    }

    return DoTransition<false>(textureUploadDescs, textureDataDescNum);
}

Result HelperDataUpload::UploadBuffers(const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum)
{
    Result result = DoTransition<true>(bufferUploadDescs, bufferUploadDescNum);
    if (result != Result::SUCCESS)
        return result;

    uint32_t i = 0;
    uint64_t bufferContentOffset = 0;

    while (i < bufferUploadDescNum)
    {
        result = BeginCommandBuffers();
        if (result != Result::SUCCESS)
            return result;

        m_UploadBufferOffset = 0;
        m_MappedMemory = (uint8_t*)NRI.MapBuffer(*m_UploadBuffer, 0, m_UploadBufferSize);

        for (; i < bufferUploadDescNum && CopyBufferContent(bufferUploadDescs[i], bufferContentOffset); i++)
            ;

        NRI.UnmapBuffer(*m_UploadBuffer);

        result = EndCommandBuffersAndSubmit();
        if (result != Result::SUCCESS)
            return result;
    }

    return DoTransition<false>(bufferUploadDescs, bufferUploadDescNum);
}

Result HelperDataUpload::BeginCommandBuffers()
{
    Result result = Result::SUCCESS;

    for (uint32_t i = 0; i < m_CommandBuffers.size() && result == Result::SUCCESS; i++)
        result = NRI.BeginCommandBuffer(*m_CommandBuffers[i], nullptr, i);

    return result;
}

Result HelperDataUpload::EndCommandBuffersAndSubmit()
{
    for (uint32_t i = 0; i < m_CommandBuffers.size(); i++)
    {
        const Result result = NRI.EndCommandBuffer(*m_CommandBuffers[i]);
        if (result != Result::SUCCESS)
            return result;
    }

    QueueSubmitDesc queueSubmitDesc = {};
    queueSubmitDesc.commandBufferNum = 1;

    for (uint32_t i = 0; i < m_CommandBuffers.size(); i++)
    {
        queueSubmitDesc.commandBuffers = &m_CommandBuffers[i];
        queueSubmitDesc.nodeIndex = i;

        NRI.QueueSubmit(m_CommandQueue, queueSubmitDesc);

        NRI.QueueSignal(m_CommandQueue, *m_Fence, m_FenceValue);
        NRI.Wait(*m_Fence, m_FenceValue);

        m_FenceValue++;
    }

    NRI.ResetCommandAllocator(*m_CommandAllocators);

    return Result::SUCCESS;
}

bool HelperDataUpload::CopyTextureContent(const TextureUploadDesc& textureUploadDesc, Dim_t& arrayOffset, Mip_t& mipOffset, bool& isCapacityInsufficient)
{
    if (textureUploadDesc.subresources == nullptr)
        return true;

    const DeviceDesc& deviceDesc = NRI.GetDeviceDesc(m_Device);

    for (; arrayOffset < textureUploadDesc.arraySize; arrayOffset++)
    {
        for (; mipOffset < textureUploadDesc.mipNum; mipOffset++)
        {
            const auto& subresource = textureUploadDesc.subresources[arrayOffset * textureUploadDesc.mipNum + mipOffset];

            const uint32_t sliceRowNum = subresource.slicePitch / subresource.rowPitch;
            const uint32_t alignedRowPitch = Align(subresource.rowPitch, deviceDesc.uploadBufferTextureRowAlignment);
            const uint32_t alignedSlicePitch = Align(sliceRowNum * alignedRowPitch, deviceDesc.uploadBufferTextureSliceAlignment);
            const uint64_t mipLevelContentSize = uint64_t(alignedSlicePitch) * subresource.sliceNum;
            const uint64_t freeSpace = m_UploadBufferSize - m_UploadBufferOffset;

            if (mipLevelContentSize > freeSpace)
            {
                isCapacityInsufficient = mipLevelContentSize > m_UploadBufferSize;
                return false;
            }

            CopyTextureSubresourceContent(subresource, alignedRowPitch, alignedSlicePitch);

            TextureDataLayoutDesc srcDataLayout = {};
            srcDataLayout.offset = m_UploadBufferOffset;
            srcDataLayout.rowPitch = alignedRowPitch;
            srcDataLayout.slicePitch = alignedSlicePitch;

            TextureRegionDesc dstRegion = {};
            dstRegion.arrayOffset = arrayOffset;
            dstRegion.mipOffset = mipOffset;

            for (uint32_t k = 0; k < m_CommandBuffers.size(); k++)
                NRI.CmdUploadBufferToTexture(*m_CommandBuffers[k], *textureUploadDesc.texture, dstRegion, *m_UploadBuffer, srcDataLayout);

            m_UploadBufferOffset = Align(m_UploadBufferOffset + mipLevelContentSize, COPY_ALIGMENT);
        }
        mipOffset = 0;
    }
    arrayOffset = 0;

    m_UploadBufferOffset = Align(m_UploadBufferOffset, COPY_ALIGMENT);

    return true;
}

void HelperDataUpload::CopyTextureSubresourceContent(const TextureSubresourceUploadDesc& subresource, uint64_t alignedRowPitch, uint64_t alignedSlicePitch)
{
    const uint32_t sliceRowNum = subresource.slicePitch / subresource.rowPitch;

    // TODO: D3D11 does not allow to call CmdUploadBufferToTexture() while the upload buffer is mapped.
    m_MappedMemory = (uint8_t*)NRI.MapBuffer(*m_UploadBuffer, m_UploadBufferOffset, subresource.sliceNum * alignedSlicePitch);

    uint8_t* slices = m_MappedMemory;
    for (uint32_t k = 0; k < subresource.sliceNum; k++)
    {
        for (uint32_t l = 0; l < sliceRowNum; l++)
        {
            uint8_t* dstRow = slices + k * alignedSlicePitch + l * alignedRowPitch;
            uint8_t* srcRow = (uint8_t*)subresource.slices + k * subresource.slicePitch + l * subresource.rowPitch;
            memcpy(dstRow, srcRow, subresource.rowPitch);
        }
    }

    NRI.UnmapBuffer(*m_UploadBuffer);
}

bool HelperDataUpload::CopyBufferContent(const BufferUploadDesc& bufferUploadDesc, uint64_t& bufferContentOffset)
{
    if (bufferUploadDesc.dataSize == 0)
        return true;

    const uint64_t freeSpace = m_UploadBufferSize - m_UploadBufferOffset;
    const uint64_t copySize = std::min(bufferUploadDesc.dataSize - bufferContentOffset, freeSpace);

    if (freeSpace == 0)
        return false;

    memcpy(m_MappedMemory + m_UploadBufferOffset, (uint8_t*)bufferUploadDesc.data + bufferContentOffset, (size_t)copySize);

    for (uint32_t j = 0; j < m_CommandBuffers.size(); j++)
    {
        NRI.CmdCopyBuffer(*m_CommandBuffers[j], *bufferUploadDesc.buffer, j, bufferUploadDesc.bufferOffset + bufferContentOffset,
            *m_UploadBuffer, 0, m_UploadBufferOffset, copySize);
    }

    bufferContentOffset += copySize;
    m_UploadBufferOffset += copySize;

    if (bufferContentOffset != bufferUploadDesc.dataSize)
        return false;

    bufferContentOffset = 0;
    m_UploadBufferOffset = Align(m_UploadBufferOffset, COPY_ALIGMENT);

    return true;
}

template<bool isInitialTransition>
Result HelperDataUpload::DoTransition(const TextureUploadDesc* textureUploadDescs, uint32_t textureDataDescNum)
{
    const Result result = BeginCommandBuffers();
    if (result != Result::SUCCESS)
        return result;

    constexpr uint32_t TEXTURES_PER_PASS = 256;
    TextureTransitionBarrierDesc textureTransitions[TEXTURES_PER_PASS];

    for (uint32_t i = 0; i < textureDataDescNum;)
    {
        const uint32_t passBegin = i;
        const uint32_t passEnd = std::min(i + TEXTURES_PER_PASS, textureDataDescNum);

        for ( ; i < passEnd; i++)
        {
            const TextureUploadDesc& textureDesc = textureUploadDescs[i];
            TextureTransitionBarrierDesc& transition = textureTransitions[i - passBegin];

            transition = {};
            transition.texture = textureDesc.texture;
            transition.mipNum = textureDesc.mipNum;
            transition.arraySize = textureDesc.arraySize;

            if (isInitialTransition)
                transition.nextState = {AccessBits::COPY_DESTINATION, TextureLayout::COPY_DESTINATION};
            else
            {
                transition.prevState = {AccessBits::COPY_DESTINATION, TextureLayout::COPY_DESTINATION};
                transition.nextState = textureDesc.nextState;
            }
        }

        TransitionBarrierDesc transitions = {};
        transitions.textures = textureTransitions;
        transitions.textureNum = passEnd - passBegin;

        for (uint32_t j = 0; j < m_CommandBuffers.size(); j++)
            NRI.CmdPipelineBarrier(*m_CommandBuffers[j], &transitions, nullptr, BarrierDependency::COPY_STAGE);
    }

    return EndCommandBuffersAndSubmit();
}

template<bool isInitialTransition>
Result HelperDataUpload::DoTransition(const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum)
{
    const Result result = BeginCommandBuffers();
    if (result != Result::SUCCESS)
        return result;

    constexpr uint32_t BUFFERS_PER_PASS = 256;
    BufferTransitionBarrierDesc bufferTransitions[BUFFERS_PER_PASS];

    for (uint32_t i = 0; i < bufferUploadDescNum;)
    {
        const uint32_t passBegin = i;
        const uint32_t passEnd = std::min(i + BUFFERS_PER_PASS, bufferUploadDescNum);

        for ( ; i < passEnd; i++)
        {
            const BufferUploadDesc& bufferUploadDesc = bufferUploadDescs[i];
            BufferTransitionBarrierDesc& bufferTransition = bufferTransitions[i - passBegin];

            bufferTransition = {};
            bufferTransition.buffer = bufferUploadDesc.buffer;

            if (isInitialTransition)
            {
                bufferTransition.prevAccess = bufferUploadDesc.prevAccess;
                bufferTransition.nextAccess = AccessBits::COPY_DESTINATION;
            }
            else
            {
                bufferTransition.prevAccess = AccessBits::COPY_DESTINATION;
                bufferTransition.nextAccess = bufferUploadDesc.nextAccess;
            }
        }

        TransitionBarrierDesc transitions = {};
        transitions.buffers = bufferTransitions;
        transitions.bufferNum = passEnd - passBegin;

        for (uint32_t j = 0; j < m_CommandBuffers.size(); j++)
            NRI.CmdPipelineBarrier(*m_CommandBuffers[j], &transitions, nullptr, BarrierDependency::COPY_STAGE);
    }

    return EndCommandBuffersAndSubmit();
}
