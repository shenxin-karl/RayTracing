// © 2021 NVIDIA Corporation

#pragma once

NRI_NAMESPACE_BEGIN

NRI_FORWARD_STRUCT(AccelerationStructure);

NRI_ENUM
(
    GeometryType, uint8_t,

    TRIANGLES,
    AABBS,

    MAX_NUM
);

NRI_ENUM
(
    AccelerationStructureType, uint8_t,

    TOP_LEVEL,
    BOTTOM_LEVEL,

    MAX_NUM
);

NRI_ENUM
(
    CopyMode, uint8_t,

    CLONE,
    COMPACT,

    MAX_NUM
);

NRI_ENUM_BITS
(
    BottomLevelGeometryBits, uint32_t,

    NONE                                = 0,
    OPAQUE_GEOMETRY                     = NRI_SET_BIT(0),
    NO_DUPLICATE_ANY_HIT_INVOCATION     = NRI_SET_BIT(1)
);

NRI_ENUM_BITS
(
    TopLevelInstanceBits, uint32_t,

    NONE                                = 0,
    TRIANGLE_CULL_DISABLE               = NRI_SET_BIT(0),
    TRIANGLE_FRONT_COUNTERCLOCKWISE     = NRI_SET_BIT(1),
    FORCE_OPAQUE                        = NRI_SET_BIT(2),
    FORCE_NON_OPAQUE                    = NRI_SET_BIT(3)
);

NRI_ENUM_BITS
(
    AccelerationStructureBuildBits, uint32_t,

    NONE                                = 0,
    ALLOW_UPDATE                        = NRI_SET_BIT(0),
    ALLOW_COMPACTION                    = NRI_SET_BIT(1),
    PREFER_FAST_TRACE                   = NRI_SET_BIT(2),
    PREFER_FAST_BUILD                   = NRI_SET_BIT(3),
    MINIMIZE_MEMORY                     = NRI_SET_BIT(4)
);

NRI_STRUCT(ShaderLibrary)
{
    const NRI_NAME(ShaderDesc)* shaderDescs;
    uint32_t shaderNum;
};

NRI_STRUCT(ShaderGroupDesc)
{
    uint32_t shaderIndices[3];
};

NRI_STRUCT(RayTracingPipelineDesc)
{
    const NRI_NAME(PipelineLayout)* pipelineLayout;
    const NRI_NAME(ShaderLibrary)* shaderLibrary;
    const NRI_NAME(ShaderGroupDesc)* shaderGroupDescs; // TODO: move to ShaderLibrary
    uint32_t shaderGroupDescNum;
    uint32_t recursionDepthMax;
    uint32_t payloadAttributeSizeMax;
    uint32_t intersectionAttributeSizeMax;
};

NRI_STRUCT(Triangles)
{
    NRI_NAME(Buffer)* vertexBuffer;
    uint64_t vertexOffset;
    uint32_t vertexNum;
    uint64_t vertexStride;
    NRI_NAME(Format) vertexFormat;
    NRI_NAME(Buffer)* indexBuffer;
    uint64_t indexOffset;
    uint32_t indexNum;
    NRI_NAME(IndexType) indexType;
    NRI_NAME(Buffer)* transformBuffer;
    uint64_t transformOffset;
};

NRI_STRUCT(AABBs)
{
    NRI_NAME(Buffer)* buffer;
    uint32_t boxNum;
    uint32_t stride;
    uint64_t offset;
};

#if defined(_MSC_VER)
    // Disable bogus nameless union warning that's treated as error
    #pragma warning(disable:4201)
#endif

NRI_STRUCT(GeometryObject)
{
    NRI_NAME(GeometryType) type;
    NRI_NAME(BottomLevelGeometryBits) flags;
    union
    {
        NRI_NAME(Triangles) triangles;
        NRI_NAME(AABBs) boxes;
    };
};

#if defined(_MSC_VER)
    #pragma warning(default:4201)
#endif

NRI_STRUCT(GeometryObjectInstance)
{
    float transform[3][4];
    uint32_t instanceId : 24;
    uint32_t mask : 8;
    uint32_t shaderBindingTableLocalOffset : 24;
    NRI_NAME(TopLevelInstanceBits) flags : 8;
    uint64_t accelerationStructureHandle;
};

NRI_STRUCT(AccelerationStructureDesc)
{
    NRI_NAME(AccelerationStructureType) type;
    NRI_NAME(AccelerationStructureBuildBits) flags;
    uint32_t instanceOrGeometryObjectNum;
    uint32_t nodeMask;
    const NRI_NAME(GeometryObject)* geometryObjects;
};

NRI_STRUCT(AccelerationStructureMemoryBindingDesc)
{
    NRI_NAME(Memory)* memory;
    NRI_NAME(AccelerationStructure)* accelerationStructure;
    uint64_t offset;
    uint32_t nodeMask;
};

NRI_STRUCT(StridedBufferRegion)
{
    const NRI_NAME(Buffer)* buffer;
    uint64_t offset;
    uint64_t size;
    uint64_t stride;
};

NRI_STRUCT(DispatchRaysDesc)
{
    NRI_NAME(StridedBufferRegion) raygenShader;
    NRI_NAME(StridedBufferRegion) missShaders;
    NRI_NAME(StridedBufferRegion) hitShaderGroups;
    NRI_NAME(StridedBufferRegion) callableShaders;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
};

NRI_STRUCT(RayTracingInterface)
{
    NRI_NAME(Result) (NRI_CALL *CreateRayTracingPipeline)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(RayTracingPipelineDesc) rayTracingPipelineDesc, NRI_NAME_REF(Pipeline*) pipeline);
    NRI_NAME(Result) (NRI_CALL *CreateAccelerationStructure)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(AccelerationStructureDesc) accelerationStructureDesc, NRI_NAME_REF(AccelerationStructure*) accelerationStructure);
    NRI_NAME(Result) (NRI_CALL *BindAccelerationStructureMemory)(NRI_NAME_REF(Device) device, const NRI_NAME(AccelerationStructureMemoryBindingDesc)* memoryBindingDescs, uint32_t memoryBindingDescNum);

    NRI_NAME(Result) (NRI_CALL *CreateAccelerationStructureDescriptor)(const NRI_NAME_REF(AccelerationStructure) accelerationStructure, uint32_t nodeMask, NRI_NAME_REF(Descriptor*) descriptor);
    void (NRI_CALL *SetAccelerationStructureDebugName)(NRI_NAME_REF(AccelerationStructure) accelerationStructure, const char* name);
    void (NRI_CALL *DestroyAccelerationStructure)(NRI_NAME_REF(AccelerationStructure) accelerationStructure);

    void (NRI_CALL *GetAccelerationStructureMemoryInfo)(const NRI_NAME_REF(AccelerationStructure) accelerationStructure, NRI_NAME_REF(MemoryDesc) memoryDesc);
    uint64_t (NRI_CALL *GetAccelerationStructureUpdateScratchBufferSize)(const NRI_NAME_REF(AccelerationStructure) accelerationStructure);
    uint64_t (NRI_CALL *GetAccelerationStructureBuildScratchBufferSize)(const NRI_NAME_REF(AccelerationStructure) accelerationStructure);
    uint64_t (NRI_CALL *GetAccelerationStructureHandle)(const NRI_NAME_REF(AccelerationStructure) accelerationStructure, uint32_t nodeIndex);
    NRI_NAME(Result) (NRI_CALL *WriteShaderGroupIdentifiers)(const NRI_NAME_REF(Pipeline) pipeline, uint32_t baseShaderGroupIndex, uint32_t shaderGroupNum, void* buffer); // TODO: add stride

    void (NRI_CALL *CmdBuildTopLevelAccelerationStructure)(NRI_NAME_REF(CommandBuffer) commandBuffer, uint32_t instanceNum, const NRI_NAME_REF(Buffer) buffer, uint64_t bufferOffset,
        NRI_NAME(AccelerationStructureBuildBits) flags, NRI_NAME_REF(AccelerationStructure) dst, NRI_NAME_REF(Buffer) scratch, uint64_t scratchOffset);
    void (NRI_CALL *CmdBuildBottomLevelAccelerationStructure)(NRI_NAME_REF(CommandBuffer) commandBuffer, uint32_t geometryObjectNum, const NRI_NAME(GeometryObject)* geometryObjects,
        NRI_NAME(AccelerationStructureBuildBits) flags, NRI_NAME_REF(AccelerationStructure) dst, NRI_NAME_REF(Buffer) scratch, uint64_t scratchOffset);
    void (NRI_CALL *CmdUpdateTopLevelAccelerationStructure)(NRI_NAME_REF(CommandBuffer) commandBuffer, uint32_t instanceNum, const NRI_NAME_REF(Buffer) buffer, uint64_t bufferOffset,
        NRI_NAME(AccelerationStructureBuildBits) flags, NRI_NAME_REF(AccelerationStructure) dst, NRI_NAME_REF(AccelerationStructure) src, NRI_NAME_REF(Buffer) scratch, uint64_t scratchOffset);
    void (NRI_CALL *CmdUpdateBottomLevelAccelerationStructure)(NRI_NAME_REF(CommandBuffer) commandBuffer, uint32_t geometryObjectNum, const NRI_NAME(GeometryObject)* geometryObjects,
        NRI_NAME(AccelerationStructureBuildBits) flags, NRI_NAME_REF(AccelerationStructure) dst, NRI_NAME_REF(AccelerationStructure) src, NRI_NAME_REF(Buffer) scratch, uint64_t scratchOffset);

    void (NRI_CALL *CmdCopyAccelerationStructure)(NRI_NAME_REF(CommandBuffer) commandBuffer, NRI_NAME_REF(AccelerationStructure) dst, NRI_NAME_REF(AccelerationStructure) src, NRI_NAME(CopyMode) copyMode);
    void (NRI_CALL *CmdWriteAccelerationStructureSize)(NRI_NAME_REF(CommandBuffer) commandBuffer, const NRI_NAME(AccelerationStructure)* const* accelerationStructures, uint32_t accelerationStructureNum, NRI_NAME_REF(QueryPool) queryPool, uint32_t queryPoolOffset);

    void (NRI_CALL *CmdDispatchRays)(NRI_NAME_REF(CommandBuffer) commandBuffer, const NRI_NAME_REF(DispatchRaysDesc) dispatchRaysDesc);

    uint64_t (NRI_CALL* GetAccelerationStructureNativeObject)(const NRI_NAME_REF(AccelerationStructure) accelerationStructure, uint32_t nodeIndex); // ID3D12Resource* or VkAccelerationStructureKHR
};

NRI_NAMESPACE_END
