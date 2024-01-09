// © 2021 NVIDIA Corporation

#pragma once

namespace nri
{
    struct DeviceBase
    {
        inline DeviceBase(const CallbackInterface& callbacks, const StdAllocator<uint8_t>& stdAllocator) :
            m_CallbackInterface(callbacks)
            , m_StdAllocator(stdAllocator)
        {}

        inline StdAllocator<uint8_t>& GetStdAllocator()
        { return m_StdAllocator; }

        void ReportMessage(nri::Message messageType, const char* file, uint32_t line, const char* format, ...) const;

        template<typename T>
        nri::Result ValidateFunctionTable(const T& table) const
        {
            const void* const* const begin = (void**)&table;
            const void* const* const end = (void**)(&table + 1);
            for (const void* const* current = begin; current != end; current++)
            {
                if (*current == nullptr)
                {
                    REPORT_ERROR(this, "Invalid function table: function #%u is NULL!", uint32_t(current - begin));
                    return nri::Result::FAILURE;
                }
            }

            return nri::Result::SUCCESS;
        }

        virtual ~DeviceBase() {}
        virtual const DeviceDesc& GetDesc() const = 0;
        virtual void Destroy() = 0;
        virtual Result FillFunctionTable(CoreInterface& table) const { table = {}; return Result::UNSUPPORTED; }
        virtual Result FillFunctionTable(SwapChainInterface& table) const { table = {}; return Result::UNSUPPORTED; }
        virtual Result FillFunctionTable(WrapperD3D11Interface& table) const { table = {}; return Result::UNSUPPORTED; }
        virtual Result FillFunctionTable(WrapperD3D12Interface& table) const { table = {}; return Result::UNSUPPORTED; }
        virtual Result FillFunctionTable(WrapperVKInterface& table) const { table = {}; return Result::UNSUPPORTED; }
        virtual Result FillFunctionTable(RayTracingInterface& table) const { table = {}; return Result::UNSUPPORTED; }
        virtual Result FillFunctionTable(MeshShaderInterface& table) const { table = {}; return Result::UNSUPPORTED; }
        virtual Result FillFunctionTable(HelperInterface& table) const { table = {}; return Result::UNSUPPORTED; }

    protected:
        nri::CallbackInterface m_CallbackInterface = {};
        StdAllocator<uint8_t> m_StdAllocator;
    };
}

#define Declare_PartiallyFillFunctionTable_Functions(API) \
    void Core_Buffer_PartiallyFillFunctionTable##API(CoreInterface& table); \
    void Core_CommandAllocator_PartiallyFillFunctionTable##API(CoreInterface& table); \
    void Core_CommandBuffer_PartiallyFillFunctionTable##API(CoreInterface& table); \
    void Core_CommandQueue_PartiallyFillFunctionTable##API(CoreInterface& table); \
    void Core_Descriptor_PartiallyFillFunctionTable##API(CoreInterface& table); \
    void Core_DescriptorPool_PartiallyFillFunctionTable##API(CoreInterface& table); \
    void Core_DescriptorSet_PartiallyFillFunctionTable##API(CoreInterface& table); \
    void Core_Fence_PartiallyFillFunctionTable##API(CoreInterface& table); \
    void Core_QueryPool_PartiallyFillFunctionTable##API(CoreInterface& table); \
    void Core_Texture_PartiallyFillFunctionTable##API(CoreInterface& table); \
    void SwapChain_PartiallyFillFunctionTable##API(SwapChainInterface& table); \
    void RayTracing_CommandBuffer_PartiallyFillFunctionTable##API(RayTracingInterface& table); \
    void RayTracing_AccelerationStructure_PartiallyFillFunctionTable##API(RayTracingInterface& table); \
    void MeshShader_CommandBuffer_PartiallyFillFunctionTable##API(MeshShaderInterface& table); \
    void Helper_CommandQueue_PartiallyFillFunctionTable##API(HelperInterface& table);

#define Define_Core_Buffer_PartiallyFillFunctionTable(API) \
void Core_Buffer_PartiallyFillFunctionTable##API(CoreInterface& table) \
{ \
    table.SetBufferDebugName = ::SetBufferDebugName; \
    table.GetBufferNativeObject = ::GetBufferNativeObject; \
    table.GetBufferMemoryInfo = ::GetBufferMemoryInfo; \
    table.MapBuffer = ::MapBuffer; \
    table.UnmapBuffer = ::UnmapBuffer; \
}

#define Define_Core_CommandAllocator_PartiallyFillFunctionTable(API) \
void Core_CommandAllocator_PartiallyFillFunctionTable##API(CoreInterface& table) \
{ \
    table.SetCommandAllocatorDebugName = ::SetCommandAllocatorDebugName; \
    table.CreateCommandBuffer = ::CreateCommandBuffer; \
    table.ResetCommandAllocator = ::ResetCommandAllocator; \
}

#define Define_Core_CommandBuffer_PartiallyFillFunctionTable(API) \
void Core_CommandBuffer_PartiallyFillFunctionTable##API(CoreInterface& table) \
{ \
    table.DestroyCommandBuffer = ::DestroyCommandBuffer; \
    table.BeginCommandBuffer = ::BeginCommandBuffer; \
    table.CmdSetDescriptorPool = ::CmdSetDescriptorPool; \
    table.CmdSetDescriptorSet = ::CmdSetDescriptorSet; \
    table.CmdSetPipelineLayout = ::CmdSetPipelineLayout; \
    table.CmdSetPipeline = ::CmdSetPipeline; \
    table.CmdSetConstants = ::CmdSetConstants; \
    table.CmdPipelineBarrier = ::CmdPipelineBarrier; \
    table.CmdBeginRendering = ::CmdBeginRendering; \
    table.CmdClearAttachments = ::CmdClearAttachments; \
    table.CmdSetViewports = ::CmdSetViewports; \
    table.CmdSetScissors = ::CmdSetScissors; \
    table.CmdSetDepthBounds = ::CmdSetDepthBounds; \
    table.CmdSetStencilReference = ::CmdSetStencilReference; \
    table.CmdSetSamplePositions = ::CmdSetSamplePositions; \
    table.CmdSetIndexBuffer = ::CmdSetIndexBuffer; \
    table.CmdSetVertexBuffers = ::CmdSetVertexBuffers; \
    table.CmdDraw = ::CmdDraw; \
    table.CmdDrawIndexed = ::CmdDrawIndexed; \
    table.CmdDrawIndirect = ::CmdDrawIndirect; \
    table.CmdDrawIndexedIndirect = ::CmdDrawIndexedIndirect; \
    table.CmdEndRendering = ::CmdEndRendering; \
    table.CmdDispatch = ::CmdDispatch; \
    table.CmdDispatchIndirect = ::CmdDispatchIndirect; \
    table.CmdBeginQuery = ::CmdBeginQuery; \
    table.CmdEndQuery = ::CmdEndQuery; \
    table.CmdCopyQueries = ::CmdCopyQueries; \
    table.CmdResetQueries = ::CmdResetQueries; \
    table.CmdBeginAnnotation = ::CmdBeginAnnotation; \
    table.CmdEndAnnotation = ::CmdEndAnnotation; \
    table.CmdClearStorageBuffer = ::CmdClearStorageBuffer; \
    table.CmdClearStorageTexture = ::CmdClearStorageTexture; \
    table.CmdCopyBuffer = ::CmdCopyBuffer; \
    table.CmdCopyTexture = ::CmdCopyTexture; \
    table.CmdUploadBufferToTexture = ::CmdUploadBufferToTexture; \
    table.CmdReadbackTextureToBuffer = ::CmdReadbackTextureToBuffer; \
    table.EndCommandBuffer = ::EndCommandBuffer; \
    table.SetCommandBufferDebugName = ::SetCommandBufferDebugName; \
    table.GetCommandBufferNativeObject = ::GetCommandBufferNativeObject; \
}

#define Define_Core_CommandQueue_PartiallyFillFunctionTable(API) \
void Core_CommandQueue_PartiallyFillFunctionTable##API(CoreInterface& table) \
{ \
    table.SetCommandQueueDebugName = ::SetCommandQueueDebugName; \
    table.QueueSubmit = ::QueueSubmit; \
}

#define Define_Core_Descriptor_PartiallyFillFunctionTable(API) \
void Core_Descriptor_PartiallyFillFunctionTable##API(CoreInterface& table) \
{ \
    table.SetDescriptorDebugName = ::SetDescriptorDebugName; \
    table.GetDescriptorNativeObject = ::GetDescriptorNativeObject; \
}

#define Define_Core_DescriptorPool_PartiallyFillFunctionTable(API) \
void Core_DescriptorPool_PartiallyFillFunctionTable##API(CoreInterface& table) \
{ \
    table.SetDescriptorPoolDebugName = ::SetDescriptorPoolDebugName; \
    table.AllocateDescriptorSets = ::AllocateDescriptorSets; \
    table.ResetDescriptorPool = ::ResetDescriptorPool; \
}

#define Define_Core_DescriptorSet_PartiallyFillFunctionTable(API) \
void Core_DescriptorSet_PartiallyFillFunctionTable##API(CoreInterface& table) \
{ \
    table.SetDescriptorSetDebugName = ::SetDescriptorSetDebugName; \
    table.UpdateDescriptorRanges = ::UpdateDescriptorRanges; \
    table.UpdateDynamicConstantBuffers = ::UpdateDynamicConstantBuffers; \
    table.CopyDescriptorSet = ::CopyDescriptorSet; \
}

#define Define_Core_Fence_PartiallyFillFunctionTable(API) \
void Core_Fence_PartiallyFillFunctionTable##API(CoreInterface& table) \
{ \
    table.GetFenceValue = ::GetFenceValue; \
    table.QueueSignal = ::QueueSignal; \
    table.QueueWait = ::QueueWait; \
    table.Wait = ::Wait; \
    table.SetFenceDebugName = ::SetFenceDebugName; \
}

#define Define_Core_QueryPool_PartiallyFillFunctionTable(API) \
void Core_QueryPool_PartiallyFillFunctionTable##API(CoreInterface& table) \
{ \
    table.SetQueryPoolDebugName = ::SetQueryPoolDebugName; \
    table.GetQuerySize = ::GetQuerySize; \
}

#define Define_Core_Texture_PartiallyFillFunctionTable(API) \
void Core_Texture_PartiallyFillFunctionTable##API(CoreInterface& table) \
{ \
    table.SetTextureDebugName = ::SetTextureDebugName; \
    table.GetTextureNativeObject = ::GetTextureNativeObject; \
    table.GetTextureMemoryInfo = ::GetTextureMemoryInfo; \
}

#define Define_SwapChain_PartiallyFillFunctionTable(API) \
void SwapChain_PartiallyFillFunctionTable##API(SwapChainInterface& table) \
{ \
    table.SetSwapChainDebugName = ::SetSwapChainDebugName; \
    table.GetSwapChainTextures = ::GetSwapChainTextures; \
    table.AcquireNextSwapChainTexture = ::AcquireNextSwapChainTexture; \
    table.SwapChainPresent = ::SwapChainPresent; \
    table.GetDisplayDesc = ::GetDisplayDesc; \
}

#define Define_RayTracing_CommandBuffer_PartiallyFillFunctionTable(API) \
void RayTracing_CommandBuffer_PartiallyFillFunctionTable##API(RayTracingInterface& table) \
{ \
    table.CmdBuildTopLevelAccelerationStructure = ::CmdBuildTopLevelAccelerationStructure; \
    table.CmdBuildBottomLevelAccelerationStructure = ::CmdBuildBottomLevelAccelerationStructure; \
    table.CmdUpdateTopLevelAccelerationStructure = ::CmdUpdateTopLevelAccelerationStructure; \
    table.CmdUpdateBottomLevelAccelerationStructure = ::CmdUpdateBottomLevelAccelerationStructure; \
    table.CmdCopyAccelerationStructure = ::CmdCopyAccelerationStructure; \
    table.CmdWriteAccelerationStructureSize = ::CmdWriteAccelerationStructureSize; \
    table.CmdDispatchRays = ::CmdDispatchRays; \
}

#define Define_RayTracing_AccelerationStructure_PartiallyFillFunctionTable(API) \
void RayTracing_AccelerationStructure_PartiallyFillFunctionTable##API(RayTracingInterface& table) \
{ \
    table.CreateAccelerationStructureDescriptor = ::CreateAccelerationStructureDescriptor; \
    table.GetAccelerationStructureMemoryInfo = ::GetAccelerationStructureMemoryInfo; \
    table.GetAccelerationStructureUpdateScratchBufferSize = ::GetAccelerationStructureUpdateScratchBufferSize; \
    table.GetAccelerationStructureBuildScratchBufferSize = ::GetAccelerationStructureBuildScratchBufferSize; \
    table.GetAccelerationStructureHandle = ::GetAccelerationStructureHandle; \
    table.SetAccelerationStructureDebugName = ::SetAccelerationStructureDebugName; \
    table.GetAccelerationStructureNativeObject = ::GetAccelerationStructureNativeObject; \
}

#define Define_MeshShader_CommandBuffer_PartiallyFillFunctionTable(API) \
void MeshShader_CommandBuffer_PartiallyFillFunctionTable##API(MeshShaderInterface& table) \
{ \
    table.CmdDispatchMeshTasks = ::CmdDispatchMeshTasks; \
}

#define Define_Helper_CommandQueue_PartiallyFillFunctionTable(API) \
void Helper_CommandQueue_PartiallyFillFunctionTable##API(HelperInterface& table) \
{ \
    table.ChangeResourceStates = ::ChangeResourceStates; \
    table.UploadData = ::UploadData; \
    table.WaitForIdle = ::WaitForIdle; \
}
