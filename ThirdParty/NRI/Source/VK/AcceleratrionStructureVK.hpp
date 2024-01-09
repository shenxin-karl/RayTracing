// © 2021 NVIDIA Corporation

#pragma region [  RayTracing  ]

static void NRI_CALL GetAccelerationStructureMemoryInfo(const AccelerationStructure& accelerationStructure, MemoryDesc& memoryDesc)
{
    ((AccelerationStructureVK&)accelerationStructure).GetMemoryInfo(memoryDesc);
}

static uint64_t NRI_CALL GetAccelerationStructureUpdateScratchBufferSize(const AccelerationStructure& accelerationStructure)
{
    return ((AccelerationStructureVK&)accelerationStructure).GetUpdateScratchBufferSize();
}

static uint64_t NRI_CALL GetAccelerationStructureBuildScratchBufferSize(const AccelerationStructure& accelerationStructure)
{
    return ((AccelerationStructureVK&)accelerationStructure).GetBuildScratchBufferSize();
}

static uint64_t NRI_CALL GetAccelerationStructureHandle(const AccelerationStructure& accelerationStructure, uint32_t nodeIndex)
{
    static_assert(sizeof(uint64_t) == sizeof(VkDeviceAddress), "type mismatch");
    return (uint64_t)((AccelerationStructureVK&)accelerationStructure).GetNativeHandle(nodeIndex);
}

static void NRI_CALL SetAccelerationStructureDebugName(AccelerationStructure& accelerationStructure, const char* name)
{
    ((AccelerationStructureVK&)accelerationStructure).SetDebugName(name);
}

static Result NRI_CALL CreateAccelerationStructureDescriptor(const AccelerationStructure& accelerationStructure, uint32_t nodeIndex, Descriptor*& descriptor)
{
    return ((AccelerationStructureVK&)accelerationStructure).CreateDescriptor(nodeIndex, descriptor);
}

static uint64_t NRI_CALL GetAccelerationStructureNativeObject(const AccelerationStructure& accelerationStructure, uint32_t nodeIndex)
{
    return uint64_t(((AccelerationStructureVK&)accelerationStructure).GetHandle(nodeIndex));
}

#pragma endregion

Define_RayTracing_AccelerationStructure_PartiallyFillFunctionTable(VK)
