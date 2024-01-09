// © 2021 NVIDIA Corporation

#pragma once

namespace nri
{

struct CommandQueueD3D11;

struct DeviceD3D11 final : public DeviceBase
{
    DeviceD3D11(const CallbackInterface& callbacks, StdAllocator<uint8_t>& stdAllocator);
    ~DeviceD3D11();

    inline const VersionedDevice& GetDevice() const
    { return m_Device; }

    inline IDXGIAdapter* GetAdapter() const
    { return m_Adapter.GetInterface(); }

    inline const VersionedContext& GetImmediateContext() const
    { return m_ImmediateContext; }

    inline const CoreInterface& GetCoreInterface() const
    { return m_CoreInterface; }

    Result Create(const DeviceCreationDesc& deviceCreationDesc, ID3D11Device* precreatedDevice, AGSContext* agsContext);

    //================================================================================================================
    // NRI
    //================================================================================================================

    inline void SetDebugName(const char* name)
    {
        SET_D3D_DEBUG_OBJECT_NAME(m_Device.ptr, name);
        SET_D3D_DEBUG_OBJECT_NAME(m_ImmediateContext.ptr, name);
    }

    Result CreateSwapChain(const SwapChainDesc& swapChainDesc, SwapChain*& swapChain);
    void DestroySwapChain(SwapChain& swapChain);

    Result GetCommandQueue(CommandQueueType commandQueueType, CommandQueue*& commandQueue);
    Result CreateCommandAllocator(const CommandQueue& commandQueue, CommandAllocator*& commandAllocator);
    Result CreateDescriptorPool(const DescriptorPoolDesc& descriptorPoolDesc, DescriptorPool*& descriptorPool);
    Result CreateBuffer(const BufferDesc& bufferDesc, Buffer*& buffer);
    Result CreateTexture(const TextureDesc& textureDesc, Texture*& texture);
    Result CreateDescriptor(const BufferViewDesc& bufferViewDesc, Descriptor*& bufferView);
    Result CreateDescriptor(const Texture1DViewDesc& textureViewDesc, Descriptor*& textureView);
    Result CreateDescriptor(const Texture2DViewDesc& textureViewDesc, Descriptor*& textureView);
    Result CreateDescriptor(const Texture3DViewDesc& textureViewDesc, Descriptor*& textureView);
    Result CreateDescriptor(const SamplerDesc& samplerDesc, Descriptor*& sampler);
    Result CreatePipelineLayout(const PipelineLayoutDesc& pipelineLayoutDesc, PipelineLayout*& pipelineLayout);
    Result CreatePipeline(const GraphicsPipelineDesc& graphicsPipelineDesc, Pipeline*& pipeline);
    Result CreatePipeline(const ComputePipelineDesc& computePipelineDesc, Pipeline*& pipeline);
    Result CreateQueryPool(const QueryPoolDesc& queryPoolDesc, QueryPool*& queryPool);
    Result CreateFence(uint64_t initialValue, Fence*& queueSemaphore);
    void DestroyCommandAllocator(CommandAllocator& commandAllocator);
    void DestroyDescriptorPool(DescriptorPool& descriptorPool);
    void DestroyBuffer(Buffer& buffer);
    void DestroyTexture(Texture& texture);
    void DestroyDescriptor(Descriptor& descriptor);
    void DestroyPipelineLayout(PipelineLayout& pipelineLayout);
    void DestroyPipeline(Pipeline& pipeline);
    void DestroyQueryPool(QueryPool& queryPool);
    void DestroyFence(Fence& fence);
    Result AllocateMemory(MemoryType memoryType, uint64_t size, Memory*& memory);
    Result BindBufferMemory(const BufferMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
    Result BindTextureMemory(const TextureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
    void FreeMemory(Memory& memory);
    FormatSupportBits GetFormatSupport(Format format) const;
    uint32_t CalculateAllocationNumber(const ResourceGroupDesc& resourceGroupDesc) const;
    Result AllocateAndBindMemory(const ResourceGroupDesc& resourceGroupDesc, Memory** allocations);

    //================================================================================================================
    // DeviceBase
    //================================================================================================================

    const DeviceDesc& GetDesc() const
    { return m_Desc; }

    void Destroy();
    Result FillFunctionTable(CoreInterface& table) const;
    Result FillFunctionTable(SwapChainInterface& table) const;
    Result FillFunctionTable(WrapperD3D11Interface& table) const;
    Result FillFunctionTable(HelperInterface& helperInterface) const;

private:
    void InitVersionedDevice(bool isDeferredContextsEmulationRequested);
    void InitVersionedContext();

    template<typename Implementation, typename Interface, typename ... Args>
    Result CreateImplementation(Interface*& entity, const Args&... args);

private:
    // don't sort - ~D3D11Extensions must be called last!
    D3D11Extensions m_Ext = {};
    VersionedDevice m_Device = {};
    VersionedContext m_ImmediateContext = {};
    Vector<CommandQueueD3D11> m_CommandQueues;
    DeviceDesc m_Desc = {};
    CRITICAL_SECTION m_CriticalSection = {};
    CoreInterface m_CoreInterface = {};
    ComPtr<IDXGIAdapter> m_Adapter;
    bool m_SkipLiveObjectsReporting = false;

private:
    void FillDesc(bool isValidationEnabled);
};

}
