// © 2021 NVIDIA Corporation

#include "SharedVK.h"
#include "PipelineVK.h"
#include "PipelineLayoutVK.h"

using namespace nri;

PipelineVK::~PipelineVK()
{
    if (!m_OwnsNativeObjects)
        return;

    const auto& vk = m_Device.GetDispatchTable();
    if (m_Handle != VK_NULL_HANDLE)
        vk.DestroyPipeline(m_Device, m_Handle, m_Device.GetAllocationCallbacks());
}

Result PipelineVK::Create(const GraphicsPipelineDesc& graphicsPipelineDesc)
{
    m_OwnsNativeObjects = true;
    m_BindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    VkPipelineVertexInputStateCreateInfo vertexInputState = {};
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
    VkPipelineTessellationStateCreateInfo tessellationState = {};
    VkPipelineViewportStateCreateInfo viewportState = {};
    VkPipelineRasterizationConservativeStateCreateInfoEXT consetvativeRasterizationState = {};
    VkPipelineRasterizationStateCreateInfo rasterizationState = {};
    VkPipelineMultisampleStateCreateInfo multisampleState = {};
    VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
    VkPipelineColorBlendStateCreateInfo colorBlendState = {};
    VkPipelineDynamicStateCreateInfo dynamicState = {};

    VkPipelineShaderStageCreateInfo* stages = STACK_ALLOC(VkPipelineShaderStageCreateInfo, graphicsPipelineDesc.shaderStageNum);
    VkShaderModule* modules = STACK_ALLOC(VkShaderModule, graphicsPipelineDesc.shaderStageNum);
    VkShaderModule* modulesBegin = modules;

    const InputAssemblyDesc& inputAssembly = *graphicsPipelineDesc.inputAssembly;
    vertexInputState.pVertexAttributeDescriptions = STACK_ALLOC(VkVertexInputAttributeDescription, inputAssembly.attributeNum);
    vertexInputState.pVertexBindingDescriptions = STACK_ALLOC(VkVertexInputBindingDescription, inputAssembly.streamNum);

    if (graphicsPipelineDesc.outputMerger != nullptr)
        colorBlendState.pAttachments = STACK_ALLOC(VkPipelineColorBlendAttachmentState, graphicsPipelineDesc.outputMerger->colorNum);

    uint32_t sampleMask = graphicsPipelineDesc.rasterization->sampleMask;
    multisampleState.pSampleMask = &sampleMask;

    for (uint32_t i = 0; i < graphicsPipelineDesc.shaderStageNum; i++)
    {
        const ShaderDesc& shaderDesc = graphicsPipelineDesc.shaderStages[i];
        Result res = SetupShaderStage(stages[i], shaderDesc, modules);
        if (res != Result::SUCCESS)
            return res;

        stages[i].pName = (shaderDesc.entryPointName == nullptr) ? "main" : shaderDesc.entryPointName;
    }

    FillVertexInputState(graphicsPipelineDesc, vertexInputState);
    FillInputAssemblyState(graphicsPipelineDesc, inputAssemblyState);
    FillTessellationState(graphicsPipelineDesc, tessellationState);
    FillViewportState(graphicsPipelineDesc, viewportState);
    FillRasterizationState(graphicsPipelineDesc, rasterizationState, consetvativeRasterizationState);
    FillMultisampleState(graphicsPipelineDesc, multisampleState);
    FillDepthStencilState(graphicsPipelineDesc, depthStencilState);
    FillColorBlendState(graphicsPipelineDesc, colorBlendState);
    FillDynamicState(dynamicState);

    const PipelineLayoutVK& pipelineLayoutVK = *(const PipelineLayoutVK*)graphicsPipelineDesc.pipelineLayout;

    VkFormat* colorFormats = graphicsPipelineDesc.outputMerger ? STACK_ALLOC(VkFormat, graphicsPipelineDesc.outputMerger->colorNum) : nullptr;

    VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo = {};
    pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    if (graphicsPipelineDesc.outputMerger)
    {
        for (uint32_t i = 0; i < graphicsPipelineDesc.outputMerger->colorNum; i++)
            colorFormats[i] = GetVkFormat(graphicsPipelineDesc.outputMerger->color[i].format);

        //pipelineRenderingCreateInfo.viewMask; // TODO
        pipelineRenderingCreateInfo.colorAttachmentCount = graphicsPipelineDesc.outputMerger->colorNum;
        pipelineRenderingCreateInfo.pColorAttachmentFormats = colorFormats;
        pipelineRenderingCreateInfo.depthAttachmentFormat = GetVkFormat(graphicsPipelineDesc.outputMerger->depthStencilFormat);
        pipelineRenderingCreateInfo.stencilAttachmentFormat = GetVkFormat(graphicsPipelineDesc.outputMerger->depthStencilFormat);
    }

    const VkGraphicsPipelineCreateInfo info = {
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        &pipelineRenderingCreateInfo,
        (VkPipelineCreateFlags)0,
        graphicsPipelineDesc.shaderStageNum,
        stages,
        &vertexInputState,
        &inputAssemblyState,
        &tessellationState,
        &viewportState,
        &rasterizationState,
        &multisampleState,
        &depthStencilState,
        &colorBlendState,
        &dynamicState, // TODO: do we need dynamic state?
        pipelineLayoutVK,
        VK_NULL_HANDLE,
        0,
        VK_NULL_HANDLE,
        -1
    };

    const auto& vk = m_Device.GetDispatchTable();
    const VkResult vkResult = vk.CreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &info, m_Device.GetAllocationCallbacks(), &m_Handle);

    RETURN_ON_FAILURE(&m_Device, vkResult == VK_SUCCESS, GetReturnCode(vkResult),
        "Can't create a graphics pipeline: vkCreateGraphicsPipelines returned %d.", (int32_t)vkResult);

    for (size_t i = 0; i < graphicsPipelineDesc.shaderStageNum; i++)
        vk.DestroyShaderModule(m_Device, modulesBegin[i], m_Device.GetAllocationCallbacks());

    return Result::SUCCESS;
}

Result PipelineVK::Create(const ComputePipelineDesc& computePipelineDesc)
{
    m_OwnsNativeObjects = true;
    m_BindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;

    const PipelineLayoutVK& pipelineLayoutVK = *(const PipelineLayoutVK*)computePipelineDesc.pipelineLayout;

    const VkShaderModuleCreateInfo moduleInfo = {
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        nullptr,
        (VkShaderModuleCreateFlags)0,
        (size_t)computePipelineDesc.computeShader.size,
        (const uint32_t*)computePipelineDesc.computeShader.bytecode
    };

    VkShaderModule module = VK_NULL_HANDLE;
    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.CreateShaderModule(m_Device, &moduleInfo, m_Device.GetAllocationCallbacks(), &module);

    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result),
        "Can't create a compute pipeline: vkCreateShaderModule returned %d.", (int32_t)result);

    VkPipelineShaderStageCreateInfo stage = {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        nullptr,
        (VkPipelineShaderStageCreateFlags)0,
        VK_SHADER_STAGE_COMPUTE_BIT,
        module,
        (computePipelineDesc.computeShader.entryPointName == nullptr) ? "main" : computePipelineDesc.computeShader.entryPointName,
        nullptr
    };

    const VkComputePipelineCreateInfo info = {
        VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        nullptr,
        (VkPipelineCreateFlags)0,
        stage,
        pipelineLayoutVK,
        VK_NULL_HANDLE,
        -1
    };

    result = vk.CreateComputePipelines(m_Device, VK_NULL_HANDLE, 1, &info, m_Device.GetAllocationCallbacks(), &m_Handle);

    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result),
        "Can't create a compute pipeline: vkCreateComputePipelines returned %d.", (int32_t)result);

    vk.DestroyShaderModule(m_Device, module, m_Device.GetAllocationCallbacks());

    return Result::SUCCESS;
}

Result PipelineVK::Create(const RayTracingPipelineDesc& rayTracingPipelineDesc)
{
    m_OwnsNativeObjects = true;
    m_BindPoint = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;

    const PipelineLayoutVK& pipelineLayoutVK = *(const PipelineLayoutVK*)rayTracingPipelineDesc.pipelineLayout;

    const uint32_t stageNum = rayTracingPipelineDesc.shaderLibrary->shaderNum;
    VkPipelineShaderStageCreateInfo* stages = STACK_ALLOC(VkPipelineShaderStageCreateInfo, stageNum);
    VkShaderModule* modules = STACK_ALLOC(VkShaderModule, stageNum);
    VkShaderModule* modulesBegin = modules;

    for (uint32_t i = 0; i < stageNum; i++)
    {
        const ShaderDesc& shaderDesc = rayTracingPipelineDesc.shaderLibrary->shaderDescs[i];
        Result result = SetupShaderStage(stages[i], shaderDesc, modules);
        if (result != Result::SUCCESS)
            return result;

        stages[i].pName = (shaderDesc.entryPointName == nullptr) ? "main" : shaderDesc.entryPointName;
    }

    VkRayTracingShaderGroupCreateInfoKHR* groupArray = STACK_ALLOC(VkRayTracingShaderGroupCreateInfoKHR, rayTracingPipelineDesc.shaderGroupDescNum);
    for (uint32_t i = 0; i < rayTracingPipelineDesc.shaderGroupDescNum; i++)
    {
        VkRayTracingShaderGroupCreateInfoKHR& dstGroup = groupArray[i];
        const ShaderGroupDesc& srcGroup = rayTracingPipelineDesc.shaderGroupDescs[i];

        dstGroup = { VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR };
        dstGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        dstGroup.generalShader = VK_SHADER_UNUSED_KHR;
        dstGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
        dstGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
        dstGroup.intersectionShader = VK_SHADER_UNUSED_KHR;

        for (uint32_t j = 0; j < GetCountOf(srcGroup.shaderIndices); j++)
        {
            if (srcGroup.shaderIndices[j] == 0)
                continue;

            const uint32_t index = srcGroup.shaderIndices[j] - 1;
            const VkPipelineShaderStageCreateInfo& stageInfo = stages[index];

            switch (stageInfo.stage)
            {
            case VK_SHADER_STAGE_RAYGEN_BIT_KHR:
            case VK_SHADER_STAGE_MISS_BIT_KHR:
            case VK_SHADER_STAGE_CALLABLE_BIT_KHR:
                dstGroup.generalShader = index;
                break;

            case VK_SHADER_STAGE_ANY_HIT_BIT_KHR:
                if (dstGroup.type != VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR)
                    dstGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
                dstGroup.anyHitShader = index;
                break;

            case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
                if (dstGroup.type != VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR)
                    dstGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
                dstGroup.closestHitShader = index;
                break;

            case VK_SHADER_STAGE_INTERSECTION_BIT_KHR:
                dstGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;
                dstGroup.intersectionShader = index;
                break;

            default:
                // already initialized
                break;
            }
        }
    }

    VkRayTracingPipelineCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR };

    createInfo.stageCount = stageNum;
    createInfo.pStages = stages;
    createInfo.maxPipelineRayRecursionDepth = rayTracingPipelineDesc.recursionDepthMax;
    createInfo.layout = pipelineLayoutVK;

    createInfo.groupCount = rayTracingPipelineDesc.shaderGroupDescNum;
    createInfo.pGroups = groupArray;

    createInfo.basePipelineIndex = -1;

    const auto& vk = m_Device.GetDispatchTable();
    const VkResult vkResult = vk.CreateRayTracingPipelinesKHR(m_Device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &createInfo,
        m_Device.GetAllocationCallbacks(), &m_Handle);

    RETURN_ON_FAILURE(&m_Device, vkResult == VK_SUCCESS, GetReturnCode(vkResult),
        "Can't create a ray tracing pipeline: vkCreateRayTracingPipelinesKHR returned %d.", (int32_t)vkResult);

    for (size_t i = 0; i < stageNum; i++)
        vk.DestroyShaderModule(m_Device, modulesBegin[i], m_Device.GetAllocationCallbacks());

    return Result::SUCCESS;
}

Result PipelineVK::CreateGraphics(NRIVkPipeline vkPipeline)
{
    m_OwnsNativeObjects = false;
    m_Handle = (VkPipeline)vkPipeline;
    m_BindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    return Result::SUCCESS;
}

Result PipelineVK::CreateCompute(NRIVkPipeline vkPipeline)
{
    m_OwnsNativeObjects = false;
    m_Handle = (VkPipeline)vkPipeline;
    m_BindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;

    return Result::SUCCESS;
}

Result PipelineVK::SetupShaderStage(VkPipelineShaderStageCreateInfo& stage, const ShaderDesc& shaderDesc, VkShaderModule*& modules)
{
    const VkShaderModuleCreateInfo moduleInfo = {
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        nullptr,
        (VkShaderModuleCreateFlags)0,
        (size_t)shaderDesc.size,
        (const uint32_t*)shaderDesc.bytecode
    };

    VkShaderModule module = VK_NULL_HANDLE;
    const auto& vk = m_Device.GetDispatchTable();
    const VkResult result = vk.CreateShaderModule(m_Device, &moduleInfo, m_Device.GetAllocationCallbacks(), &module);

    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result),
        "Can't create a graphics shader module: vkCreateShaderModule returned %d.", (int32_t)result);

    *(modules++) = module;

    stage = {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        nullptr,
        (VkPipelineShaderStageCreateFlags)0,
        (VkShaderStageFlagBits)GetShaderStageFlags(shaderDesc.stage),
        module,
        nullptr,
        nullptr
    };

    return Result::SUCCESS;
}

void PipelineVK::FillVertexInputState(const GraphicsPipelineDesc& graphicsPipelineDesc, VkPipelineVertexInputStateCreateInfo& state) const
{
    state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    const InputAssemblyDesc& inputAssembly = *graphicsPipelineDesc.inputAssembly;
    state.vertexAttributeDescriptionCount = inputAssembly.attributeNum;
    state.vertexBindingDescriptionCount = inputAssembly.streamNum;

    VkVertexInputAttributeDescription* attributes = const_cast<VkVertexInputAttributeDescription*>(state.pVertexAttributeDescriptions);
    for (uint32_t i = 0; i < inputAssembly.attributeNum; i++)
    {
        const VertexAttributeDesc& attribute_desc = inputAssembly.attributes[i];
        attributes[i] = {
            (uint32_t)i,
            attribute_desc.streamIndex,
            GetVkFormat(attribute_desc.format),
            attribute_desc.offset
        };
    }

    VkVertexInputBindingDescription* streams = const_cast<VkVertexInputBindingDescription*>(state.pVertexBindingDescriptions);
    for (uint32_t i = 0; i < inputAssembly.streamNum; i++)
    {
        const VertexStreamDesc& stream_desc = inputAssembly.streams[i];
        streams[i] = {
           stream_desc.bindingSlot,
           stream_desc.stride,
           (stream_desc.stepRate == VertexStreamStepRate::PER_VERTEX) ? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE
        };
    }
}

void PipelineVK::FillInputAssemblyState(const GraphicsPipelineDesc& graphicsPipelineDesc, VkPipelineInputAssemblyStateCreateInfo& state) const
{
    state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

    const InputAssemblyDesc& inputAssembly = *graphicsPipelineDesc.inputAssembly;
    state.topology = GetTopology(inputAssembly.topology);
    state.primitiveRestartEnable = inputAssembly.primitiveRestart != PrimitiveRestart::DISABLED;
}

void PipelineVK::FillTessellationState(const GraphicsPipelineDesc& graphicsPipelineDesc, VkPipelineTessellationStateCreateInfo& state) const
{
    state.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    state.patchControlPoints = graphicsPipelineDesc.inputAssembly->tessControlPointNum;
}

void PipelineVK::FillViewportState(const GraphicsPipelineDesc& graphicsPipelineDesc, VkPipelineViewportStateCreateInfo& state)
{
    state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;

    state.viewportCount = 1;
    state.scissorCount = 1;

    if (graphicsPipelineDesc.rasterization == nullptr)
        return;

    state.viewportCount = graphicsPipelineDesc.rasterization->viewportNum;
    state.scissorCount = graphicsPipelineDesc.rasterization->viewportNum;
}

void PipelineVK::FillRasterizationState(const GraphicsPipelineDesc& graphicsPipelineDesc, VkPipelineRasterizationStateCreateInfo& state, VkPipelineRasterizationConservativeStateCreateInfoEXT& conservativeRasterState) const
{
    state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

    if (graphicsPipelineDesc.rasterization == nullptr)
        return;

    const RasterizationDesc& rasterization = *graphicsPipelineDesc.rasterization;
    state.depthClampEnable = rasterization.depthClamp;
    state.rasterizerDiscardEnable = rasterization.rasterizerDiscard;
    state.polygonMode = GetPolygonMode(rasterization.fillMode);
    state.cullMode = GetCullMode(rasterization.cullMode);
    state.frontFace = rasterization.frontCounterClockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
    state.depthBiasEnable = rasterization.depthBiasConstantFactor != 0.0f || rasterization.depthBiasSlopeFactor != 0.0f;
    state.depthBiasConstantFactor = GetDepthOffset(rasterization.depthBiasConstantFactor);
    state.depthBiasClamp = rasterization.depthBiasClamp;
    state.depthBiasSlopeFactor = rasterization.depthBiasSlopeFactor;
    state.lineWidth = 1.0f;

    if (graphicsPipelineDesc.rasterization->conservativeRasterization == false)
        return;

    conservativeRasterState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT;
    conservativeRasterState.conservativeRasterizationMode = VK_CONSERVATIVE_RASTERIZATION_MODE_OVERESTIMATE_EXT;
    conservativeRasterState.extraPrimitiveOverestimationSize = 0.0f;
    state.pNext = &conservativeRasterState;
}

void PipelineVK::FillMultisampleState(const GraphicsPipelineDesc& graphicsPipelineDesc, VkPipelineMultisampleStateCreateInfo& state) const
{
    state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

    if (graphicsPipelineDesc.rasterization == nullptr)
        return;

    const RasterizationDesc& rasterization = *graphicsPipelineDesc.rasterization;
    state.rasterizationSamples = (VkSampleCountFlagBits)rasterization.sampleNum;
    state.sampleShadingEnable = false;
    state.minSampleShading = 0.0f;
    *const_cast<VkSampleMask*>(state.pSampleMask) = rasterization.sampleMask;
    state.alphaToCoverageEnable = rasterization.alphaToCoverage;
    state.alphaToOneEnable = false;
}

void PipelineVK::FillDepthStencilState(const GraphicsPipelineDesc& graphicsPipelineDesc, VkPipelineDepthStencilStateCreateInfo& state) const
{
    state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

    if (graphicsPipelineDesc.outputMerger == nullptr)
        return;

    const DepthAttachmentDesc& depthDesc = graphicsPipelineDesc.outputMerger->depth;
    const StencilAttachmentDesc& stencil = graphicsPipelineDesc.outputMerger->stencil;
    state.depthTestEnable = depthDesc.compareFunc != CompareFunc::NONE;
    state.depthWriteEnable = depthDesc.write;
    state.depthCompareOp = GetCompareOp(depthDesc.compareFunc);
    state.depthBoundsTestEnable = VK_TRUE;
    state.stencilTestEnable = stencil.front.compareFunc != CompareFunc::NONE;
    state.minDepthBounds = 0.0f;
    state.maxDepthBounds = 1.0f;

    state.front.failOp = GetStencilOp(stencil.front.fail);
    state.front.passOp = GetStencilOp(stencil.front.pass);
    state.front.depthFailOp = GetStencilOp(stencil.front.depthFail);
    state.front.compareOp = GetCompareOp(stencil.front.compareFunc);
    state.front.compareMask = stencil.compareMask;
    state.front.writeMask = stencil.writeMask;
    state.front.reference = stencil.reference;

    state.back.failOp = GetStencilOp(stencil.back.fail);
    state.back.passOp = GetStencilOp(stencil.back.pass);
    state.back.depthFailOp = GetStencilOp(stencil.back.depthFail);
    state.back.compareOp = GetCompareOp(stencil.back.compareFunc);
    state.back.compareMask = stencil.compareMask;
    state.back.writeMask = stencil.writeMask;
    state.back.reference = stencil.reference;
}

void PipelineVK::FillColorBlendState(const GraphicsPipelineDesc& graphicsPipelineDesc, VkPipelineColorBlendStateCreateInfo& state) const
{
    state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

    if (graphicsPipelineDesc.outputMerger == nullptr)
        return;

    const OutputMergerDesc& outputMerger = *graphicsPipelineDesc.outputMerger;

    state.logicOpEnable = outputMerger.colorLogicFunc != LogicFunc::NONE;
    state.logicOp = GetLogicOp(outputMerger.colorLogicFunc);
    state.attachmentCount = outputMerger.colorNum;

    for (uint32_t i = 0; i < 4; i++)
        state.blendConstants[i] = *(&outputMerger.blendConsts.x + i);

    VkPipelineColorBlendAttachmentState* attachments = const_cast<VkPipelineColorBlendAttachmentState*>(state.pAttachments);
    for (uint32_t i = 0; i < outputMerger.colorNum; i++)
    {
        const ColorAttachmentDesc& attachmentDesc = outputMerger.color[i];

        attachments[i] = {
            VkBool32(attachmentDesc.blendEnabled),
            GetBlendFactor(attachmentDesc.colorBlend.srcFactor),
            GetBlendFactor(attachmentDesc.colorBlend.dstFactor),
            GetBlendOp(attachmentDesc.colorBlend.func),
            GetBlendFactor(attachmentDesc.alphaBlend.srcFactor),
            GetBlendFactor(attachmentDesc.alphaBlend.dstFactor),
            GetBlendOp(attachmentDesc.alphaBlend.func),
            GetColorComponent(attachmentDesc.colorWriteMask)
        };
    }
}

constexpr std::array<VkDynamicState, 4> DYNAMIC_STATE = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR,
    VK_DYNAMIC_STATE_DEPTH_BOUNDS,
    VK_DYNAMIC_STATE_STENCIL_REFERENCE,
};

void PipelineVK::FillDynamicState(VkPipelineDynamicStateCreateInfo& state) const
{
    state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;

    state.dynamicStateCount = (uint32_t)DYNAMIC_STATE.size();
    state.pDynamicStates = DYNAMIC_STATE.data();
}

void PipelineVK::FillGroupIndices(const RayTracingPipelineDesc& rayTracingPipelineDesc, uint32_t* groupIndices)
{
    for (uint32_t i = 0; i < rayTracingPipelineDesc.shaderGroupDescNum; i++)
    {
        const ShaderGroupDesc& shaderGroupDesc = rayTracingPipelineDesc.shaderGroupDescs[i];
        for (uint32_t j = 0; i < GetCountOf(shaderGroupDesc.shaderIndices); j++)
        {
            if (shaderGroupDesc.shaderIndices[j] != 0)
                groupIndices[shaderGroupDesc.shaderIndices[j] - 1] = j;
        }
    }
}

//================================================================================================================
// NRI
//================================================================================================================

void PipelineVK::SetDebugName(const char* name)
{
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_PIPELINE, (uint64_t)m_Handle, name);
}

inline Result PipelineVK::WriteShaderGroupIdentifiers(uint32_t baseShaderGroupIndex, uint32_t shaderGroupNum, void* buffer) const
{
    const size_t dataSize = (size_t)(shaderGroupNum * m_Device.GetDesc().rayTracingShaderGroupIdentifierSize);

    const auto& vk = m_Device.GetDispatchTable();
    const VkResult result = vk.GetRayTracingShaderGroupHandlesKHR(m_Device, m_Handle, baseShaderGroupIndex, shaderGroupNum, dataSize, buffer);

    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result),
        "Can't get shader group identifiers: vkGetRayTracingShaderGroupHandlesKHR returned %d.", (int32_t)result);

    return Result::SUCCESS;
}

#include "PipelineVK.hpp"