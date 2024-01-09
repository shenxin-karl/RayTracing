// © 2021 NVIDIA Corporation

#pragma once

namespace nri
{

struct PipelineLayoutD3D11;
struct PipelineD3D11;
struct BufferD3D11;

struct CommandBufferD3D11 final : public CommandBufferHelper
{
    CommandBufferD3D11(DeviceD3D11& device);
    ~CommandBufferD3D11();

    inline DeviceD3D11& GetDevice() const
    { return m_Device; }

    inline operator ID3D11CommandList*() const
    { return m_CommandList.GetInterface(); }

    //================================================================================================================
    // CommandBufferHelper
    //================================================================================================================

    Result Create(ID3D11DeviceContext* precreatedContext);
    void Submit();
    ID3D11DeviceContext* GetNativeObject() const;
    StdAllocator<uint8_t>& GetStdAllocator() const;

    //================================================================================================================
    // NRI
    //================================================================================================================

    inline void SetDebugName(const char* name)
    {
        SET_D3D_DEBUG_OBJECT_NAME(m_DeferredContext.ptr, name);
        SET_D3D_DEBUG_OBJECT_NAME(m_CommandList, name);
    }

    Result Begin(const DescriptorPool* descriptorPool);
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
    inline void EndRendering() {}
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
    void CopyBuffer(Buffer& dstBuffer, uint64_t dstOffset, const Buffer& srcBuffer, uint64_t srcOffset, uint64_t size);
    void CopyTexture(Texture& dstTexture, const TextureRegionDesc* dstRegionDesc, const Texture& srcTexture, const TextureRegionDesc* srcRegionDesc);
    void UploadBufferToTexture(Texture& dstTexture, const TextureRegionDesc& dstRegionDesc, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayoutDesc);
    void ReadbackTextureToBuffer(Buffer& dstBuffer, TextureDataLayoutDesc& dstDataLayoutDesc, const Texture& srcTexture, const TextureRegionDesc& srcRegionDesc);
    void Dispatch(uint32_t x, uint32_t y, uint32_t z);
    void DispatchIndirect(const Buffer& buffer, uint64_t offset);
    void PipelineBarrier(const TransitionBarrierDesc* transitionBarriers, const AliasingBarrierDesc* aliasingBarriers, BarrierDependency dependency);
    void BeginQuery(const QueryPool& queryPool, uint32_t offset);
    void EndQuery(const QueryPool& queryPool, uint32_t offset);
    void CopyQueries(const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& dstBuffer, uint64_t dstOffset);
    void BeginAnnotation(const char* name);
    void EndAnnotation();

private:
    DeviceD3D11& m_Device;
    VersionedContext m_DeferredContext = {}; // can be immediate to redirect data from emulation
    BindingState m_BindingState = {};
    SamplePositionsState m_SamplePositionsState = {};
    ComPtr<ID3D11CommandList> m_CommandList;
    ComPtr<ID3DUserDefinedAnnotation> m_Annotation;
    std::array<ID3D11RenderTargetView*, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT> m_RenderTargets = {};
    ID3D11DepthStencilView* m_DepthStencil = nullptr;
    PipelineLayoutD3D11* m_PipelineLayout = nullptr;
    PipelineD3D11* m_Pipeline = nullptr;
    const Buffer* m_IndexBuffer = nullptr;
    const Buffer* m_VertexBuffer = nullptr;
    uint64_t m_IndexBufferOffset = 0;
    uint64_t m_VertexBufferOffset = 0;
    uint32_t m_VertexBufferBaseSlot = 0;
    uint32_t m_RenderTargetNum = 0;
    IndexType m_IndexType = IndexType::UINT32;
    float m_DepthBounds[2] = {0.0f, 1.0f};
    uint8_t m_StencilRef = 0;
};

}
