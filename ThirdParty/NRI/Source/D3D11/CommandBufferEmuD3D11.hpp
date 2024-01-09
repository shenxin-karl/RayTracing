// © 2021 NVIDIA Corporation

#pragma region [  Core  ]

static void NRI_CALL SetCommandBufferDebugName(CommandBuffer& commandBuffer, const char* name)
{
    ((CommandBufferEmuD3D11&)commandBuffer).SetDebugName(name);
}

static Result NRI_CALL BeginCommandBuffer(CommandBuffer& commandBuffer, const DescriptorPool* descriptorPool, uint32_t nodeIndex)
{
    MaybeUnused(nodeIndex);

    return ((CommandBufferEmuD3D11&)commandBuffer).Begin(descriptorPool);
}

static Result NRI_CALL EndCommandBuffer(CommandBuffer& commandBuffer)
{
    return ((CommandBufferEmuD3D11&)commandBuffer).End();
}

static void NRI_CALL CmdSetPipelineLayout(CommandBuffer& commandBuffer, const PipelineLayout& pipelineLayout)
{
    ((CommandBufferEmuD3D11&)commandBuffer).SetPipelineLayout(pipelineLayout);
}

static void NRI_CALL CmdSetPipeline(CommandBuffer& commandBuffer, const Pipeline& pipeline)
{
    ((CommandBufferEmuD3D11&)commandBuffer).SetPipeline(pipeline);
}

static void NRI_CALL CmdPipelineBarrier(CommandBuffer& commandBuffer, const TransitionBarrierDesc* transitionBarriers, const AliasingBarrierDesc* aliasingBarriers, BarrierDependency dependency)
{
    ((CommandBufferEmuD3D11&)commandBuffer).PipelineBarrier(transitionBarriers, aliasingBarriers, dependency);
}

static void NRI_CALL CmdSetDescriptorPool(CommandBuffer& commandBuffer, const DescriptorPool& descriptorPool)
{
    ((CommandBufferEmuD3D11&)commandBuffer).SetDescriptorPool(descriptorPool);
}

static void NRI_CALL CmdSetDescriptorSet(CommandBuffer& commandBuffer, uint32_t setIndexInPipelineLayout, const DescriptorSet& descriptorSet, const uint32_t* dynamicConstantBufferOffsets)
{
    ((CommandBufferEmuD3D11&)commandBuffer).SetDescriptorSet(setIndexInPipelineLayout, descriptorSet, dynamicConstantBufferOffsets);
}

static void NRI_CALL CmdSetConstants(CommandBuffer& commandBuffer, uint32_t pushConstantIndex, const void* data, uint32_t size)
{
    ((CommandBufferEmuD3D11&)commandBuffer).SetConstants(pushConstantIndex, data, size);
}

static void NRI_CALL CmdBeginRendering(CommandBuffer& commandBuffer, const AttachmentsDesc& attachmentsDesc)
{
    ((CommandBufferEmuD3D11&)commandBuffer).BeginRendering(attachmentsDesc);
}

static void NRI_CALL CmdEndRendering(CommandBuffer& commandBuffer)
{
    ((CommandBufferEmuD3D11&)commandBuffer).EndRendering();
}

static void NRI_CALL CmdSetViewports(CommandBuffer& commandBuffer, const Viewport* viewports, uint32_t viewportNum)
{
    ((CommandBufferEmuD3D11&)commandBuffer).SetViewports(viewports, viewportNum);
}

static void NRI_CALL CmdSetScissors(CommandBuffer& commandBuffer, const Rect* rects, uint32_t rectNum)
{
    ((CommandBufferEmuD3D11&)commandBuffer).SetScissors(rects, rectNum);
}

static void NRI_CALL CmdSetDepthBounds(CommandBuffer& commandBuffer, float boundsMin, float boundsMax)
{
    ((CommandBufferEmuD3D11&)commandBuffer).SetDepthBounds(boundsMin, boundsMax);
}

static void NRI_CALL CmdSetStencilReference(CommandBuffer& commandBuffer, uint8_t reference)
{
    ((CommandBufferEmuD3D11&)commandBuffer).SetStencilReference(reference);
}

static void NRI_CALL CmdSetSamplePositions(CommandBuffer& commandBuffer, const SamplePosition* positions, uint32_t positionNum)
{
    ((CommandBufferEmuD3D11&)commandBuffer).SetSamplePositions(positions, positionNum);
}

static void NRI_CALL CmdClearAttachments(CommandBuffer& commandBuffer, const ClearDesc* clearDescs, uint32_t clearDescNum, const Rect* rects, uint32_t rectNum)
{
    ((CommandBufferEmuD3D11&)commandBuffer).ClearAttachments(clearDescs, clearDescNum, rects, rectNum);
}

static void NRI_CALL CmdSetIndexBuffer(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset, IndexType indexType)
{
    ((CommandBufferEmuD3D11&)commandBuffer).SetIndexBuffer(buffer, offset, indexType);
}

static void NRI_CALL CmdSetVertexBuffers(CommandBuffer& commandBuffer, uint32_t baseSlot, uint32_t bufferNum, const Buffer* const* buffers, const uint64_t* offsets)
{
    ((CommandBufferEmuD3D11&)commandBuffer).SetVertexBuffers(baseSlot, bufferNum, buffers, offsets);
}

static void NRI_CALL CmdDraw(CommandBuffer& commandBuffer, uint32_t vertexNum, uint32_t instanceNum, uint32_t baseVertex, uint32_t baseInstance)
{
    ((CommandBufferEmuD3D11&)commandBuffer).Draw(vertexNum, instanceNum, baseVertex, baseInstance);
}

static void NRI_CALL CmdDrawIndexed(CommandBuffer& commandBuffer, uint32_t indexNum, uint32_t instanceNum, uint32_t baseIndex, uint32_t baseVertex, uint32_t baseInstance)
{
    ((CommandBufferEmuD3D11&)commandBuffer).DrawIndexed(indexNum, instanceNum, baseIndex, baseVertex, baseInstance);
}

static void NRI_CALL CmdDrawIndirect(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride)
{
    ((CommandBufferEmuD3D11&)commandBuffer).DrawIndirect(buffer, offset, drawNum, stride);
}

static void NRI_CALL CmdDrawIndexedIndirect(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride)
{
    ((CommandBufferEmuD3D11&)commandBuffer).DrawIndexedIndirect(buffer, offset, drawNum, stride);
}

static void NRI_CALL CmdDispatch(CommandBuffer& commandBuffer, uint32_t x, uint32_t y, uint32_t z)
{
    ((CommandBufferEmuD3D11&)commandBuffer).Dispatch(x, y, z);
}

static void NRI_CALL CmdDispatchIndirect(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset)
{
    ((CommandBufferEmuD3D11&)commandBuffer).DispatchIndirect(buffer, offset);
}

static void NRI_CALL CmdBeginQuery(CommandBuffer& commandBuffer, const QueryPool& queryPool, uint32_t offset)
{
    ((CommandBufferEmuD3D11&)commandBuffer).BeginQuery(queryPool, offset);
}

static void NRI_CALL CmdEndQuery(CommandBuffer& commandBuffer, const QueryPool& queryPool, uint32_t offset)
{
    ((CommandBufferEmuD3D11&)commandBuffer).EndQuery(queryPool, offset);
}

static void NRI_CALL CmdBeginAnnotation(CommandBuffer& commandBuffer, const char* name)
{
    ((CommandBufferEmuD3D11&)commandBuffer).BeginAnnotation(name);
}

static void NRI_CALL CmdEndAnnotation(CommandBuffer& commandBuffer)
{
    ((CommandBufferEmuD3D11&)commandBuffer).EndAnnotation();
}

static void NRI_CALL CmdClearStorageBuffer(CommandBuffer& commandBuffer, const ClearStorageBufferDesc& clearDesc)
{
    ((CommandBufferEmuD3D11&)commandBuffer).ClearStorageBuffer(clearDesc);
}

static void NRI_CALL CmdClearStorageTexture(CommandBuffer& commandBuffer, const ClearStorageTextureDesc& clearDesc)
{
    ((CommandBufferEmuD3D11&)commandBuffer).ClearStorageTexture(clearDesc);
}

static void NRI_CALL CmdCopyBuffer(CommandBuffer& commandBuffer, Buffer& dstBuffer, uint32_t dstNodeIndex, uint64_t dstOffset,
    const Buffer& srcBuffer, uint32_t srcNodeIndex, uint64_t srcOffset, uint64_t size)
{
    MaybeUnused(dstNodeIndex);
    MaybeUnused(srcNodeIndex);

    ((CommandBufferEmuD3D11&)commandBuffer).CopyBuffer(dstBuffer, dstOffset, srcBuffer, srcOffset, size);
}

static void NRI_CALL CmdCopyTexture(CommandBuffer& commandBuffer, Texture& dstTexture, uint32_t dstNodeIndex,
    const TextureRegionDesc* dstRegionDesc, const Texture& srcTexture, uint32_t srcNodeIndex, const TextureRegionDesc* srcRegionDesc)
{
    MaybeUnused(dstNodeIndex);
    MaybeUnused(srcNodeIndex);

    ((CommandBufferEmuD3D11&)commandBuffer).CopyTexture(dstTexture, dstRegionDesc, srcTexture, srcRegionDesc);
}

static void NRI_CALL CmdUploadBufferToTexture(CommandBuffer& commandBuffer, Texture& dstTexture, const TextureRegionDesc& dstRegionDesc, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayoutDesc)
{
    ((CommandBufferEmuD3D11&)commandBuffer).UploadBufferToTexture(dstTexture, dstRegionDesc, srcBuffer, srcDataLayoutDesc);
}

static void NRI_CALL CmdReadbackTextureToBuffer(CommandBuffer& commandBuffer, Buffer& dstBuffer, TextureDataLayoutDesc& dstDataLayoutDesc, const Texture& srcTexture, const TextureRegionDesc& srcRegionDesc)
{
    ((CommandBufferEmuD3D11&)commandBuffer).ReadbackTextureToBuffer(dstBuffer, dstDataLayoutDesc, srcTexture, srcRegionDesc);
}

static void NRI_CALL CmdCopyQueries(CommandBuffer& commandBuffer, const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& dstBuffer, uint64_t dstOffset)
{
    ((CommandBufferEmuD3D11&)commandBuffer).CopyQueries(queryPool, offset, num, dstBuffer, dstOffset);
}

static void NRI_CALL CmdResetQueries(CommandBuffer&, const QueryPool&, uint32_t, uint32_t)
{
}

static void NRI_CALL DestroyCommandBuffer(CommandBuffer& commandBuffer)
{
    if(!(&commandBuffer))
        return;

    CommandBufferHelper& commandBufferHelper = (CommandBufferHelper&)commandBuffer;
    Deallocate(commandBufferHelper.GetStdAllocator(), &commandBufferHelper);
}

static void* NRI_CALL GetCommandBufferNativeObject(const CommandBuffer& commandBuffer)
{
    if (!(&commandBuffer))
        return nullptr;

    CommandBufferHelper& commandBufferHelper = (CommandBufferHelper&)commandBuffer;
    return commandBufferHelper.GetNativeObject();
}

#pragma endregion

void Core_CommandBufferEmu_PartiallyFillFunctionTable(CoreInterface& coreInterface)
{
    coreInterface.DestroyCommandBuffer = ::DestroyCommandBuffer;
    coreInterface.BeginCommandBuffer = ::BeginCommandBuffer;
    coreInterface.CmdSetDescriptorPool = ::CmdSetDescriptorPool;
    coreInterface.CmdSetDescriptorSet = ::CmdSetDescriptorSet;
    coreInterface.CmdSetPipelineLayout = ::CmdSetPipelineLayout;
    coreInterface.CmdSetPipeline = ::CmdSetPipeline;
    coreInterface.CmdSetConstants = ::CmdSetConstants;
    coreInterface.CmdPipelineBarrier = ::CmdPipelineBarrier;
    coreInterface.CmdBeginRendering = ::CmdBeginRendering;
    coreInterface.CmdClearAttachments = ::CmdClearAttachments;
    coreInterface.CmdSetViewports = ::CmdSetViewports;
    coreInterface.CmdSetScissors = ::CmdSetScissors;
    coreInterface.CmdSetDepthBounds = ::CmdSetDepthBounds;
    coreInterface.CmdSetStencilReference = ::CmdSetStencilReference;
    coreInterface.CmdSetSamplePositions = ::CmdSetSamplePositions;
    coreInterface.CmdSetIndexBuffer = ::CmdSetIndexBuffer;
    coreInterface.CmdSetVertexBuffers = ::CmdSetVertexBuffers;
    coreInterface.CmdDraw = ::CmdDraw;
    coreInterface.CmdDrawIndexed = ::CmdDrawIndexed;
    coreInterface.CmdDrawIndirect = ::CmdDrawIndirect;
    coreInterface.CmdDrawIndexedIndirect = ::CmdDrawIndexedIndirect;
    coreInterface.CmdEndRendering = ::CmdEndRendering;
    coreInterface.CmdDispatch = ::CmdDispatch;
    coreInterface.CmdDispatchIndirect = ::CmdDispatchIndirect;
    coreInterface.CmdBeginQuery = ::CmdBeginQuery;
    coreInterface.CmdEndQuery = ::CmdEndQuery;
    coreInterface.CmdCopyQueries = ::CmdCopyQueries;
    coreInterface.CmdResetQueries = ::CmdResetQueries;
    coreInterface.CmdBeginAnnotation = ::CmdBeginAnnotation;
    coreInterface.CmdEndAnnotation = ::CmdEndAnnotation;
    coreInterface.CmdClearStorageBuffer = ::CmdClearStorageBuffer;
    coreInterface.CmdClearStorageTexture = ::CmdClearStorageTexture;
    coreInterface.CmdCopyBuffer = ::CmdCopyBuffer;
    coreInterface.CmdCopyTexture = ::CmdCopyTexture;
    coreInterface.CmdUploadBufferToTexture = ::CmdUploadBufferToTexture;
    coreInterface.CmdReadbackTextureToBuffer = ::CmdReadbackTextureToBuffer;
    coreInterface.EndCommandBuffer = ::EndCommandBuffer;
    coreInterface.SetCommandBufferDebugName = ::SetCommandBufferDebugName;
    coreInterface.GetCommandBufferNativeObject = ::GetCommandBufferNativeObject;
}
