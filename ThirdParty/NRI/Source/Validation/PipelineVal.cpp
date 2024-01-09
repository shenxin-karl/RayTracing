// © 2021 NVIDIA Corporation

#include "SharedExternal.h"
#include "DeviceBase.h"
#include "DeviceVal.h"
#include "SharedVal.h"
#include "PipelineVal.h"

using namespace nri;

PipelineVal::PipelineVal(DeviceVal& device, Pipeline* pipeline) : DeviceObjectVal(device, pipeline), m_RayTracingAPI(device.GetRayTracingInterface()) {
}

PipelineVal::PipelineVal(DeviceVal& device, Pipeline* pipeline, const GraphicsPipelineDesc& graphicsPipelineDesc)
    : DeviceObjectVal(device, pipeline), m_RayTracingAPI(device.GetRayTracingInterface()), m_PipelineLayout(graphicsPipelineDesc.pipelineLayout) {
}

PipelineVal::PipelineVal(DeviceVal& device, Pipeline* pipeline, const ComputePipelineDesc& computePipelineDesc)
    : DeviceObjectVal(device, pipeline), m_RayTracingAPI(device.GetRayTracingInterface()), m_PipelineLayout(computePipelineDesc.pipelineLayout) {
}

PipelineVal::PipelineVal(DeviceVal& device, Pipeline* pipeline, const RayTracingPipelineDesc& rayTracingPipelineDesc)
    : DeviceObjectVal(device, pipeline), m_RayTracingAPI(device.GetRayTracingInterface()), m_PipelineLayout(rayTracingPipelineDesc.pipelineLayout) {
}

void PipelineVal::SetDebugName(const char* name) {
    m_Name = name;
    GetCoreInterface().SetPipelineDebugName(*GetImpl(), name);
}

Result PipelineVal::WriteShaderGroupIdentifiers(uint32_t baseShaderGroupIndex, uint32_t shaderGroupNum, void* buffer) {
    return m_RayTracingAPI.WriteShaderGroupIdentifiers(*GetImpl(), baseShaderGroupIndex, shaderGroupNum, buffer);
}

#include "PipelineVal.hpp"
