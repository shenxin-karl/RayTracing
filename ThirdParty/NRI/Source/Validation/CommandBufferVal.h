// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct CommandBufferVal : public DeviceObjectVal<CommandBuffer> {
    CommandBufferVal(DeviceVal& device, CommandBuffer* commandBuffer, bool isWrapped)
        : DeviceObjectVal(device, commandBuffer),
          m_ValidationCommands(device.GetStdAllocator()),
          m_RayTracingAPI(device.GetRayTracingInterface()),
          m_MeshShaderAPI(device.GetMeshShaderInterface()),
          m_IsRecordingStarted(isWrapped),
          m_IsWrapped(isWrapped) {
    }

    inline const Vector<uint8_t>& GetValidationCommands() const {
        return m_ValidationCommands;
    }

    inline void* GetNativeObject() const {
        return GetCoreInterface().GetCommandBufferNativeObject(*GetImpl());
    }

    //================================================================================================================
    // NRI
    //================================================================================================================
    void SetDebugName(const char* name);
    Result Begin(const DescriptorPool* descriptorPool, uint32_t nodeIndex);
    Result End();
    void SetViewports(const Viewport* viewports, uint32_t viewportNum);
    void SetScissors(const Rect* rects, uint32_t rectNum);
    void SetDepthBounds(float boundsMin, float boundsMax);
    void SetStencilReference(uint8_t reference);
    void SetSamplePositions(const SamplePosition* positions, uint32_t positionNum);
    void ClearAttachments(const ClearDesc* clearDescs, uint32_t clearDescNum, const Rect* rects, uint32_t rectNum);
    void ClearStorageBuffer(const ClearStorageBufferDesc& clearDesc);
    void ClearStorageTexture(const ClearStorageTextureDesc& clearDesc);
    void BeginRendering(const AttachmentsDesc& attachmentsDesc);
    void EndRendering();
    void SetVertexBuffers(uint32_t baseSlot, uint32_t bufferNum, const Buffer* const* buffers, const uint64_t* offsets);
    void SetIndexBuffer(const Buffer& buffer, uint64_t offset, IndexType indexType);
    void SetPipelineLayout(const PipelineLayout& pipelineLayout);
    void SetPipeline(const Pipeline& pipeline);
    void SetDescriptorPool(const DescriptorPool& descriptorPool);
    void SetDescriptorSet(uint32_t setIndexInPipelineLayout, const DescriptorSet& descriptorSet, const uint32_t* dynamicConstantBufferOffsets);
    void SetConstants(uint32_t pushConstantIndex, const void* data, uint32_t size);
    void Draw(uint32_t vertexNum, uint32_t instanceNum, uint32_t baseVertex, uint32_t baseInstance);
    void DrawIndexed(uint32_t indexNum, uint32_t instanceNum, uint32_t baseIndex, uint32_t baseVertex, uint32_t baseInstance);
    void DrawIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride);
    void DrawIndexedIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride);
    void CopyBuffer(Buffer& dstBuffer, uint32_t dstNodeIndex, uint64_t dstOffset, const Buffer& srcBuffer, uint32_t srcNodeIndex, uint64_t srcOffset, uint64_t size);
    void CopyTexture(
        Texture& dstTexture, uint32_t dstNodeIndex, const TextureRegionDesc* dstRegionDesc, const Texture& srcTexture, uint32_t srcNodeIndex, const TextureRegionDesc* srcRegionDesc
    );
    void UploadBufferToTexture(Texture& dstTexture, const TextureRegionDesc& dstRegionDesc, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayoutDesc);
    void ReadbackTextureToBuffer(Buffer& dstBuffer, TextureDataLayoutDesc& dstDataLayoutDesc, const Texture& srcTexture, const TextureRegionDesc& srcRegionDesc);
    void Dispatch(uint32_t x, uint32_t y, uint32_t z);
    void DispatchIndirect(const Buffer& buffer, uint64_t offset);
    void PipelineBarrier(const TransitionBarrierDesc* transitionBarriers, const AliasingBarrierDesc* aliasingBarriers, BarrierDependency dependency);
    void BeginQuery(const QueryPool& queryPool, uint32_t offset);
    void EndQuery(const QueryPool& queryPool, uint32_t offset);
    void CopyQueries(const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& dstBuffer, uint64_t dstOffset);
    void ResetQueries(const QueryPool& queryPool, uint32_t offset, uint32_t num);
    void BeginAnnotation(const char* name);
    void EndAnnotation();
    void Destroy();

    void BuildTopLevelAccelerationStructure(
        uint32_t instanceNum, const Buffer& buffer, uint64_t bufferOffset, AccelerationStructureBuildBits flags, AccelerationStructure& dst, Buffer& scratch, uint64_t scratchOffset
    );
    void BuildBottomLevelAccelerationStructure(
        uint32_t geometryObjectNum, const GeometryObject* geometryObjects, AccelerationStructureBuildBits flags, AccelerationStructure& dst, Buffer& scratch, uint64_t scratchOffset
    );
    void UpdateTopLevelAccelerationStructure(
        uint32_t instanceNum, const Buffer& buffer, uint64_t bufferOffset, AccelerationStructureBuildBits flags, AccelerationStructure& dst, AccelerationStructure& src,
        Buffer& scratch, uint64_t scratchOffset
    );
    void UpdateBottomLevelAccelerationStructure(
        uint32_t geometryObjectNum, const GeometryObject* geometryObjects, AccelerationStructureBuildBits flags, AccelerationStructure& dst, AccelerationStructure& src,
        Buffer& scratch, uint64_t scratchOffset
    );

    void CopyAccelerationStructure(AccelerationStructure& dst, AccelerationStructure& src, CopyMode copyMode);
    void WriteAccelerationStructureSize(const AccelerationStructure* const* accelerationStructures, uint32_t accelerationStructureNum, QueryPool& queryPool, uint32_t queryOffset);

    void DispatchRays(const DispatchRaysDesc& dispatchRaysDesc);
    void DispatchMeshTasks(uint32_t x, uint32_t y, uint32_t z);

  private:
    template <typename Command>
    Command& AllocateValidationCommand();

    Vector<uint8_t> m_ValidationCommands;
    const RayTracingInterface& m_RayTracingAPI;
    const MeshShaderInterface& m_MeshShaderAPI;
    int32_t m_AnnotationStack = 0;
    bool m_IsRecordingStarted = false;
    bool m_IsWrapped = false;
    bool m_IsRenderPass = false;
};

enum class ValidationCommandType : uint32_t {
    NONE,
    BEGIN_QUERY,
    END_QUERY,
    RESET_QUERY,
    MAX_NUM
};

struct ValidationCommandUseQuery {
    ValidationCommandType type;
    QueryPool* queryPool;
    uint32_t queryPoolOffset;
};

struct ValidationCommandResetQuery {
    ValidationCommandType type;
    QueryPool* queryPool;
    uint32_t queryPoolOffset;
    uint32_t queryNum;
};

} // namespace nri
