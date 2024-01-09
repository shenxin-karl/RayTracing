// © 2021 NVIDIA Corporation

#pragma once

#include "DescriptorSetD3D12.h"

namespace nri
{

struct DeviceD3D12;
struct PipelineLayoutD3D12;
struct CommandBufferD3D12;

struct PipelineD3D12
{
    inline PipelineD3D12(DeviceD3D12& device)
        : m_Device(device)
        , m_ShaderGroupNames(device.GetStdAllocator())
    {}

    inline ~PipelineD3D12()
    {}

    inline operator ID3D12PipelineState*() const
    { return m_PipelineState.GetInterface(); }

    inline bool IsGraphicsPipeline() const
    { return m_IsGraphicsPipeline; }

    inline DeviceD3D12& GetDevice() const
    { return m_Device; }

    inline const PipelineLayoutD3D12& GetPipelineLayout() const
    { return *m_PipelineLayout; }

    inline uint32_t GetIAStreamStride(uint32_t streamSlot) const
    { return m_IAStreamStride[streamSlot]; }

    inline Sample_t GetSampleNum() const
    { return m_SampleNum; }

    Result Create(const GraphicsPipelineDesc& graphicsPipelineDesc);
    Result Create(const ComputePipelineDesc& computePipelineDesc);
    Result Create(const RayTracingPipelineDesc& rayTracingPipelineDesc);

    void Bind(ID3D12GraphicsCommandList* graphicsCommandList, D3D12_PRIMITIVE_TOPOLOGY& primitiveTopology) const;

    //================================================================================================================
    // NRI
    //================================================================================================================

    inline void SetDebugName(const char* name)
    { SET_D3D_DEBUG_OBJECT_NAME(m_PipelineState, name); }

    Result WriteShaderGroupIdentifiers(uint32_t baseShaderGroupIndex, uint32_t shaderGroupNum, void* buffer) const;

private:
    Result CreateFromStream(const GraphicsPipelineDesc& graphicsPipelineDesc);
    void FillInputLayout(D3D12_INPUT_LAYOUT_DESC& inputLayoutDesc, const GraphicsPipelineDesc& graphicsPipelineDesc);
    void FillShaderBytecode(D3D12_SHADER_BYTECODE& shaderBytecode, const ShaderDesc& shaderDesc) const;
    void FillRasterizerState(D3D12_RASTERIZER_DESC& rasterizerDesc, const GraphicsPipelineDesc& graphicsPipelineDesc);
    void FillDepthStencilState(D3D12_DEPTH_STENCIL_DESC& depthStencilDesc, const OutputMergerDesc& outputMergerDesc) const;
    void FillBlendState(D3D12_BLEND_DESC& blendDesc, const GraphicsPipelineDesc& graphicsPipelineDesc);
    void FillSampleDesc(DXGI_SAMPLE_DESC& sampleDesc, UINT& sampleMask, const GraphicsPipelineDesc& graphicsPipelineDesc);

private:
    DeviceD3D12& m_Device;
    std::array<uint16_t, D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT> m_IAStreamStride = {}; // TODO: optimize?
    ComPtr<ID3D12PipelineState> m_PipelineState;
    ComPtr<ID3D12StateObject> m_StateObject;
    ComPtr<ID3D12StateObjectProperties> m_StateObjectProperties;
    Vector<std::wstring> m_ShaderGroupNames;
    const PipelineLayoutD3D12* m_PipelineLayout = nullptr;
    D3D_PRIMITIVE_TOPOLOGY m_PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    Color32f m_BlendFactor = {};
    Sample_t m_SampleNum = 1;
    bool m_BlendEnabled = false;
    bool m_IsGraphicsPipeline = false;
};

}
