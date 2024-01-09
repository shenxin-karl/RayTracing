// © 2021 NVIDIA Corporation

#pragma once

struct IDXGIAdapter;

namespace nri
{

struct CommandQueueVK;

struct DeviceVK final : public DeviceBase
{
    inline operator VkDevice() const
    { return m_Device; }

    inline operator VkPhysicalDevice() const
    { return m_PhysicalDevices.front(); }

    inline operator VkInstance() const
    { return m_Instance; }

    inline const DispatchTable& GetDispatchTable() const
    { return m_VK; }

    inline const VkAllocationCallbacks* GetAllocationCallbacks() const
    { return m_AllocationCallbackPtr; }

    inline const std::array<uint32_t, COMMAND_QUEUE_TYPE_NUM>& GetQueueFamilyIndices() const
    { return m_FamilyIndices; }

    inline const SPIRVBindingOffsets& GetSPIRVBindingOffsets() const
    { return m_SPIRVBindingOffsets; }

    inline const CoreInterface& GetCoreInterface() const
    { return m_CoreInterface; }

    inline uint32_t GetPhysicalDeviceGroupSize() const
    { return m_Desc.nodeNum; }

    inline bool IsConcurrentSharingModeEnabledForBuffers() const
    { return m_ConcurrentSharingModeQueueIndices.size() > 1; }

    inline bool IsConcurrentSharingModeEnabledForImages() const
    { return m_ConcurrentSharingModeQueueIndices.size() > 1; }

    inline const Vector<uint32_t>& GetConcurrentSharingModeQueueIndices() const
    { return m_ConcurrentSharingModeQueueIndices; }

    DeviceVK(const CallbackInterface& callbacks, const StdAllocator<uint8_t>& stdAllocator);
    ~DeviceVK();

    Result Create(const DeviceCreationVKDesc& deviceCreationVKDesc);
    Result Create(const DeviceCreationDesc& deviceCreationDesc);
    bool GetMemoryType(MemoryLocation memoryLocation, uint32_t memoryTypeMask, MemoryTypeInfo& memoryTypeInfo) const;
    bool GetMemoryType(uint32_t index, MemoryTypeInfo& memoryTypeInfo) const;
    void SetDebugNameToTrivialObject(VkObjectType objectType, uint64_t handle, const char* name);
    void SetDebugNameToDeviceGroupObject(VkObjectType objectType, const uint64_t* handles, const char* name);

    //================================================================================================================
    // NRI
    //================================================================================================================

    void SetDebugName(const char* name);
    Result GetCommandQueue(CommandQueueType commandQueueType, CommandQueue*& commandQueue);
    Result CreateCommandAllocator(const CommandQueue& commandQueue, CommandAllocator*& commandAllocator);
    Result CreateDescriptorPool(const DescriptorPoolDesc& descriptorPoolDesc, DescriptorPool*& descriptorPool);
    Result CreateBuffer(const BufferDesc& bufferDesc, Buffer*& buffer);
    Result CreateTexture(const TextureDesc& textureDesc, Texture*& texture);
    Result CreateBufferView(const BufferViewDesc& bufferViewDesc, Descriptor*& bufferView);
    Result CreateTexture1DView(const Texture1DViewDesc& textureViewDesc, Descriptor*& textureView);
    Result CreateTexture2DView(const Texture2DViewDesc& textureViewDesc, Descriptor*& textureView);
    Result CreateTexture3DView(const Texture3DViewDesc& textureViewDesc, Descriptor*& textureView);
    Result CreateSampler(const SamplerDesc& samplerDesc, Descriptor*& sampler);
    Result CreatePipelineLayout(const PipelineLayoutDesc& pipelineLayoutDesc, PipelineLayout*& pipelineLayout);
    Result CreatePipeline(const GraphicsPipelineDesc& graphicsPipelineDesc, Pipeline*& pipeline);
    Result CreatePipeline(const ComputePipelineDesc& computePipelineDesc, Pipeline*& pipeline);
    Result CreateQueryPool(const QueryPoolDesc& queryPoolDesc, QueryPool*& queryPool);
    Result CreateFence(uint64_t initialValue, Fence*& fence);
    Result CreateSwapChain(const SwapChainDesc& swapChainDesc, SwapChain*& swapChain);
    Result CreatePipeline(const RayTracingPipelineDesc& rayTracingPipelineDesc, Pipeline*& pipeline);
    Result CreateAccelerationStructure(const AccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure);
    Result CreateCommandQueue(const CommandQueueVKDesc& commandQueueDesc, CommandQueue*& commandQueue);
    Result CreateCommandAllocator(const CommandAllocatorVKDesc& commandAllocatorDesc, CommandAllocator*& commandAllocator);
    Result CreateCommandBuffer(const CommandBufferVKDesc& commandBufferDesc, CommandBuffer*& commandBuffer);
    Result CreateDescriptorPool(const DescriptorPoolVKDesc& descriptorPoolVKDesc, DescriptorPool*& descriptorPool);
    Result CreateBuffer(const BufferVKDesc& bufferDesc, Buffer*& buffer);
    Result CreateTexture(const TextureVKDesc& textureVKDesc, Texture*& texture);
    Result CreateMemory(const MemoryVKDesc& memoryVKDesc, Memory*& memory);
    Result CreateGraphicsPipeline(NRIVkPipeline vkPipeline, Pipeline*& pipeline);
    Result CreateComputePipeline(NRIVkPipeline vkPipeline, Pipeline*& pipeline);
    Result CreateQueryPool(const QueryPoolVKDesc& queryPoolVKDesc, QueryPool*& queryPool);
    Result CreateAccelerationStructure(const AccelerationStructureVKDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure);
    void DestroyCommandAllocator(CommandAllocator& commandAllocator);
    void DestroyDescriptorPool(DescriptorPool& descriptorPool);
    void DestroyBuffer(Buffer& buffer);
    void DestroyTexture(Texture& texture);
    void DestroyDescriptor(Descriptor& descriptor);
    void DestroyPipelineLayout(PipelineLayout& pipelineLayout);
    void DestroyPipeline(Pipeline& pipeline);
    void DestroyQueryPool(QueryPool& queryPool);
    void DestroyFence(Fence& fence);
    void DestroySwapChain(SwapChain& swapChain);
    void DestroyAccelerationStructure(AccelerationStructure& accelerationStructure);
    Result AllocateMemory(uint32_t nodeMask, MemoryType memoryType, uint64_t size, Memory*& memory);
    Result BindBufferMemory(const BufferMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
    Result BindTextureMemory(const TextureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
    Result BindAccelerationStructureMemory(const AccelerationStructureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
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
    Result FillFunctionTable(WrapperVKInterface& wrapperVKInterface) const;
    Result FillFunctionTable(RayTracingInterface& rayTracingInterface) const;
    Result FillFunctionTable(MeshShaderInterface& meshShaderInterface) const;
    Result FillFunctionTable(HelperInterface& helperInterface) const;

private:
    Result CreateInstance(const DeviceCreationDesc& deviceCreationDesc);
    Result FindPhysicalDeviceGroup(const AdapterDesc* physicalDeviceGroup, bool enableMGPU);
    Result CreateLogicalDevice(const DeviceCreationDesc& deviceCreationDesc);
    void FillFamilyIndices(bool useEnabledFamilyIndices, const uint32_t* enabledFamilyIndices, uint32_t familyIndexNum);
    void FillDesc(bool enableValidation);
    void CreateCommandQueues();
    Result ResolvePreInstanceDispatchTable();
    Result ResolveInstanceDispatchTable();
    Result ResolveDispatchTable();
    void FilterInstanceLayers(Vector<const char*>& layers);
    void ReportDeviceGroupInfo();

    template< typename Implementation, typename Interface, typename ... Args >
    Result CreateImplementation(Interface*& entity, const Args&... args);

private:
    Lock m_Lock;
    Vector<VkPhysicalDevice> m_PhysicalDevices;
    Vector<uint32_t> m_PhysicalDeviceIndices;
    Vector<uint32_t> m_ConcurrentSharingModeQueueIndices;
    std::array<uint32_t, COMMAND_QUEUE_TYPE_NUM> m_FamilyIndices = {};
    std::array<CommandQueueVK*, COMMAND_QUEUE_TYPE_NUM> m_Queues = {};
    DispatchTable m_VK = {};
    DeviceDesc m_Desc = {};
    VkPhysicalDeviceMemoryProperties m_MemoryProps = {};
    VkAllocationCallbacks m_AllocationCallbacks = {};
    SPIRVBindingOffsets m_SPIRVBindingOffsets = {};
    CoreInterface m_CoreInterface = {};
    Library* m_Loader = nullptr;
    VkDevice m_Device = VK_NULL_HANDLE;
    VkInstance m_Instance = VK_NULL_HANDLE;
    VkAllocationCallbacks* m_AllocationCallbackPtr = nullptr;
    VkDebugUtilsMessengerEXT m_Messenger = VK_NULL_HANDLE;
#ifdef _WIN32
    ComPtr<IDXGIAdapter> m_Adapter;
#endif
    bool m_OwnsNativeObjects = false;

public:
    struct SupportedFeatures
    {
        bool debugUtils = false;
        bool subsetAllocation = false;
        bool descriptorIndexing = false;
        bool bufferDeviceAddress = false;
        bool sampleLocations = false;
        bool conservativeRaster = false;
        bool rayTracing = false;
        bool microMap = false;
        bool meshShader = false;
    } supportedFeatures;
};

}
