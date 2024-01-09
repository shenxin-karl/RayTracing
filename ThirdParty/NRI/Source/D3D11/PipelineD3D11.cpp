// © 2021 NVIDIA Corporation

#include "SharedD3D11.h"
#include "PipelineD3D11.h"

using namespace nri;

PipelineD3D11::~PipelineD3D11()
{
    Deallocate(m_Device.GetStdAllocator(), m_RasterizerStateExDesc);
}

Result PipelineD3D11::Create(const GraphicsPipelineDesc& pipelineDesc)
{
    const InputAssemblyDesc& ia = *pipelineDesc.inputAssembly;
    const RasterizationDesc& rs = *pipelineDesc.rasterization;
    const OutputMergerDesc& om = *pipelineDesc.outputMerger;
    const DepthAttachmentDesc& ds = om.depth;
    const StencilAttachmentDesc& ss = om.stencil;
    const ShaderDesc* vertexShader = nullptr;
    HRESULT hr;

    // shaders

    for (uint32_t i = 0; i < pipelineDesc.shaderStageNum; i++)
    {
        const ShaderDesc* shaderDesc = pipelineDesc.shaderStages + i;

        if (shaderDesc->stage == ShaderStage::VERTEX)
        {
            vertexShader = shaderDesc;
            hr = m_Device.GetDevice()->CreateVertexShader(shaderDesc->bytecode, (size_t)shaderDesc->size, nullptr, &m_VertexShader);
            RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateVertexShader()");
        }
        else if (shaderDesc->stage == ShaderStage::TESS_CONTROL)
        {
            hr = m_Device.GetDevice()->CreateHullShader(shaderDesc->bytecode, (size_t)shaderDesc->size, nullptr, &m_TessControlShader);
            RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateHullShader()");
        }
        else if (shaderDesc->stage == ShaderStage::TESS_EVALUATION)
        {
            hr = m_Device.GetDevice()->CreateDomainShader(shaderDesc->bytecode, (size_t)shaderDesc->size, nullptr, &m_TessEvaluationShader);
            RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateDomainShader()");
        }
        else if (shaderDesc->stage == ShaderStage::GEOMETRY)
        {
            hr = m_Device.GetDevice()->CreateGeometryShader(shaderDesc->bytecode, (size_t)shaderDesc->size, nullptr, &m_GeometryShader);
            RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateGeometryShader()");
        }
        else if (shaderDesc->stage == ShaderStage::FRAGMENT)
        {
            hr = m_Device.GetDevice()->CreatePixelShader(shaderDesc->bytecode, (size_t)shaderDesc->size, nullptr, &m_FragmentShader);
            RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreatePixelShader()");
        }
        else
            return Result::UNSUPPORTED;
    }

    // resources

    m_PipelineLayout = (const PipelineLayoutD3D11*)pipelineDesc.pipelineLayout;

    // input assembly

    m_Topology = GetD3D11TopologyFromTopology(ia.topology, ia.tessControlPointNum);

    if (ia.attributes)
    {
        uint32_t maxBindingSlot = 0;
        for (uint32_t i = 0; i < ia.streamNum; i++)
        {
            const VertexStreamDesc& stream = ia.streams[i];
            if (stream.bindingSlot > maxBindingSlot )
                maxBindingSlot = stream.bindingSlot;
        }
        m_InputAssemplyStrides.resize(maxBindingSlot + 1);

        D3D11_INPUT_ELEMENT_DESC* inputElements = STACK_ALLOC(D3D11_INPUT_ELEMENT_DESC, ia.attributeNum);

        for (uint32_t i = 0; i < ia.attributeNum; i++)
        {
            const VertexAttributeDesc& attrIn = ia.attributes[i];
            const VertexStreamDesc& stream = ia.streams[attrIn.streamIndex];
            D3D11_INPUT_ELEMENT_DESC& attrOut = inputElements[i];
            const DxgiFormat& dxgiFormat = GetDxgiFormat(attrIn.format);

            attrOut.SemanticName = attrIn.d3d.semanticName;
            attrOut.SemanticIndex = attrIn.d3d.semanticIndex;
            attrOut.Format = dxgiFormat.typed;
            attrOut.InputSlot = stream.bindingSlot;
            attrOut.AlignedByteOffset = attrIn.offset;
            attrOut.InstanceDataStepRate = stream.stepRate == VertexStreamStepRate::PER_VERTEX ? 0 : 1;
            attrOut.InputSlotClass = stream.stepRate == VertexStreamStepRate::PER_VERTEX ?
                D3D11_INPUT_PER_VERTEX_DATA : D3D11_INPUT_PER_INSTANCE_DATA;

            m_InputAssemplyStrides[stream.bindingSlot] = stream.stride;
        };

        assert(vertexShader != nullptr);
        hr = m_Device.GetDevice()->CreateInputLayout(&inputElements[0], ia.attributeNum, vertexShader->bytecode, (size_t)vertexShader->size, &m_InputLayout);
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateInputLayout()");
    }

    // rasterization

    D3D11_RASTERIZER_DESC2 rasterizerDesc = {};
    rasterizerDesc.FillMode = rs.fillMode == FillMode::SOLID ? D3D11_FILL_SOLID : D3D11_FILL_WIREFRAME;
    rasterizerDesc.CullMode = GetD3D11CullModeFromCullMode(rs.cullMode);
    rasterizerDesc.FrontCounterClockwise = rs.frontCounterClockwise;
    rasterizerDesc.DepthBias = rs.depthBiasConstantFactor;
    rasterizerDesc.DepthBiasClamp = rs.depthBiasClamp;
    rasterizerDesc.SlopeScaledDepthBias = rs.depthBiasSlopeFactor;
    rasterizerDesc.DepthClipEnable = rs.depthClamp;
    rasterizerDesc.ScissorEnable = TRUE;
    rasterizerDesc.MultisampleEnable = rs.sampleNum > 1 ? TRUE : FALSE;
    rasterizerDesc.AntialiasedLineEnable = rs.antialiasedLines;
    rasterizerDesc.ConservativeRaster = rs.conservativeRasterization ?
        D3D11_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D11_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    RasterizerState rasterizerState;
    if (m_Device.GetDevice().version >= 3)
    {
        hr = m_Device.GetDevice()->CreateRasterizerState2(&rasterizerDesc, &rasterizerState.ptr);
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device3::CreateRasterizerState2()");
    }
    else
    {
        hr = m_Device.GetDevice()->CreateRasterizerState((D3D11_RASTERIZER_DESC*)&rasterizerDesc, (ID3D11RasterizerState**)&rasterizerState.ptr);
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateRasterizerState()");
    }

    m_RasterizerStateExDesc = Allocate<NvAPI_D3D11_RASTERIZER_DESC_EX>(m_Device.GetStdAllocator());
    memset(m_RasterizerStateExDesc, 0, sizeof(*m_RasterizerStateExDesc));
    memcpy(m_RasterizerStateExDesc, &rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));
    m_RasterizerStateExDesc->ConservativeRasterEnable = rs.conservativeRasterization;
    m_RasterizerStateExDesc->ProgrammableSamplePositionsEnable = true;
    m_RasterizerStateExDesc->SampleCount = rs.sampleNum;

    m_RasterizerStates.push_back(rasterizerState);
    m_RasterizerState = rasterizerState.ptr;

    // depth-stencil

    const bool isDepthWrite = rs.rasterizerDiscard ? false : ds.write;

    D3D11_DEPTH_STENCIL_DESC depthStencilState = {};
    depthStencilState.DepthEnable = ds.compareFunc == CompareFunc::NONE ? FALSE : TRUE;
    depthStencilState.DepthWriteMask = isDepthWrite ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    depthStencilState.DepthFunc = GetD3D11ComparisonFuncFromCompareFunc(ds.compareFunc);
    depthStencilState.StencilEnable = ss.front.compareFunc == CompareFunc::NONE ? FALSE : TRUE;
    depthStencilState.StencilReadMask = ss.compareMask;
    depthStencilState.StencilWriteMask = ss.writeMask;

    depthStencilState.FrontFace.StencilFailOp = GetD3D11StencilOpFromStencilFunc(ss.front.fail);
    depthStencilState.FrontFace.StencilDepthFailOp = GetD3D11StencilOpFromStencilFunc(ss.front.depthFail);
    depthStencilState.FrontFace.StencilPassOp = GetD3D11StencilOpFromStencilFunc(ss.front.pass);
    depthStencilState.FrontFace.StencilFunc = GetD3D11ComparisonFuncFromCompareFunc(ss.front.compareFunc);

    depthStencilState.BackFace.StencilFailOp = GetD3D11StencilOpFromStencilFunc(ss.front.fail);
    depthStencilState.BackFace.StencilDepthFailOp = GetD3D11StencilOpFromStencilFunc(ss.front.depthFail);
    depthStencilState.BackFace.StencilPassOp = GetD3D11StencilOpFromStencilFunc(ss.front.pass);
    depthStencilState.BackFace.StencilFunc = GetD3D11ComparisonFuncFromCompareFunc(ss.back.compareFunc);

    hr = m_Device.GetDevice()->CreateDepthStencilState(&depthStencilState, &m_DepthStencilState);
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateDepthStencilState()");

    // output merger

    D3D11_BLEND_DESC1 blendState1 = {};
    blendState1.AlphaToCoverageEnable = rs.alphaToCoverage;
    blendState1.IndependentBlendEnable = TRUE;
    for (uint32_t i = 0; i < om.colorNum; i++)
    {
        const ColorAttachmentDesc& bs = om.color[i];
        const uint8_t colorWriteMask = rs.rasterizerDiscard ? 0 : uint8_t(bs.colorWriteMask);

        blendState1.RenderTarget[i].BlendEnable = bs.blendEnabled;
        blendState1.RenderTarget[i].SrcBlend = GetD3D11BlendFromBlendFactor(bs.colorBlend.srcFactor);
        blendState1.RenderTarget[i].DestBlend = GetD3D11BlendFromBlendFactor(bs.colorBlend.dstFactor);
        blendState1.RenderTarget[i].BlendOp = GetD3D11BlendOpFromBlendFunc(bs.colorBlend.func);
        blendState1.RenderTarget[i].SrcBlendAlpha = GetD3D11BlendFromBlendFactor(bs.alphaBlend.srcFactor);
        blendState1.RenderTarget[i].DestBlendAlpha = GetD3D11BlendFromBlendFactor(bs.alphaBlend.dstFactor);
        blendState1.RenderTarget[i].BlendOpAlpha = GetD3D11BlendOpFromBlendFunc(bs.alphaBlend.func);
        blendState1.RenderTarget[i].RenderTargetWriteMask = colorWriteMask;
        blendState1.RenderTarget[i].LogicOpEnable = om.colorLogicFunc == LogicFunc::NONE ? FALSE : TRUE;
        blendState1.RenderTarget[i].LogicOp = GetD3D11LogicOpFromLogicFunc(om.colorLogicFunc);
    }

    if (m_Device.GetDevice().version >= 1)
        hr = m_Device.GetDevice()->CreateBlendState1(&blendState1, &m_BlendState);
    else
    {
        D3D11_BLEND_DESC blendState = {};
        blendState.AlphaToCoverageEnable = blendState1.AlphaToCoverageEnable;
        blendState.IndependentBlendEnable = blendState1.IndependentBlendEnable;
        for (uint32_t i = 0; i < om.colorNum; i++)
        {
            blendState.RenderTarget[i].BlendEnable = blendState1.RenderTarget[i].BlendEnable;
            blendState.RenderTarget[i].SrcBlend = blendState1.RenderTarget[i].SrcBlend;
            blendState.RenderTarget[i].DestBlend = blendState1.RenderTarget[i].DestBlend;
            blendState.RenderTarget[i].BlendOp = blendState1.RenderTarget[i].BlendOp;
            blendState.RenderTarget[i].SrcBlendAlpha = blendState1.RenderTarget[i].SrcBlendAlpha;
            blendState.RenderTarget[i].DestBlendAlpha = blendState1.RenderTarget[i].DestBlendAlpha;
            blendState.RenderTarget[i].BlendOpAlpha = blendState1.RenderTarget[i].BlendOpAlpha;
            blendState.RenderTarget[i].RenderTargetWriteMask = blendState1.RenderTarget[i].RenderTargetWriteMask;
        }

        hr = m_Device.GetDevice()->CreateBlendState(&blendState, (ID3D11BlendState**)&m_BlendState);
    }

    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device1::CreateBlendState1()");

    m_BlendFactor = om.blendConsts;
    m_SampleMask = rs.sampleMask;
    m_IsRasterizerDiscarded = rs.rasterizerDiscard;

    return Result::SUCCESS;
}

Result PipelineD3D11::Create(const ComputePipelineDesc& pipelineDesc)
{
    if (pipelineDesc.computeShader.bytecode)
    {
        HRESULT hr = m_Device.GetDevice()->CreateComputeShader(pipelineDesc.computeShader.bytecode, (size_t)pipelineDesc.computeShader.size, nullptr, &m_ComputeShader);

        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateComputeShader()");
    }

    m_PipelineLayout = (const PipelineLayoutD3D11*)pipelineDesc.pipelineLayout;

    return Result::SUCCESS;
}

void PipelineD3D11::ChangeSamplePositions(const VersionedContext& deferredContext, const SamplePositionsState& samplePositionState, DynamicState mode)
{
    if (IsCompute())
        return;

    size_t i = 0;
    for (; i < m_RasterizerStates.size(); i++)
    {
        if (m_RasterizerStates[i].samplePositionHash == samplePositionState.positionHash)
            break;
    }

    if (i == m_RasterizerStates.size())
    {
        RasterizerState newState = {};
        newState.samplePositionHash = samplePositionState.positionHash;

        m_RasterizerStateExDesc->InterleavedSamplingEnable = samplePositionState.positionNum > m_RasterizerStateExDesc->SampleCount;
        for (uint32_t j = 0; j < samplePositionState.positionNum; j++)
        {
            m_RasterizerStateExDesc->SamplePositionsX[j] = samplePositionState.positions[j].x + 8;
            m_RasterizerStateExDesc->SamplePositionsY[j] = samplePositionState.positions[j].y + 8;
        }

        if (deferredContext.ext->IsNvAPIAvailable())
        {
            NvAPI_Status result = NvAPI_D3D11_CreateRasterizerState(m_Device.GetDevice().ptr, m_RasterizerStateExDesc, (ID3D11RasterizerState**)&newState.ptr);
            if (result != NVAPI_OK)
                REPORT_ERROR(&m_Device, "NvAPI_D3D11_CreateRasterizerState() - FAILED!");
        }
        else
            REPORT_ERROR(&m_Device, "Programmable Sample Locations feature is only supported on NVIDIA GPUs on DX11! Ignoring...");

        if (!newState.ptr)
            newState.ptr = m_RasterizerState;

        m_RasterizerStates.push_back(newState);
    }

    ID3D11RasterizerState2* newState = m_RasterizerStates[i].ptr;
    if (mode == DynamicState::BIND_AND_SET && m_RasterizerState != newState)
        deferredContext->RSSetState(newState);

    m_RasterizerState = newState;
}

void PipelineD3D11::ChangeStencilReference(const VersionedContext& deferredContext, uint8_t stencilRef, DynamicState mode)
{
    if (mode == DynamicState::BIND_AND_SET && m_StencilRef != stencilRef)
        deferredContext->OMSetDepthStencilState(m_DepthStencilState, stencilRef);

    m_StencilRef = stencilRef;
}

void PipelineD3D11::Bind(const VersionedContext& deferredContext, const PipelineD3D11* currentPipeline) const
{
    if (this == currentPipeline)
        return;

    if (IsCompute())
    {
        if (!currentPipeline || m_ComputeShader != currentPipeline->m_ComputeShader)
            deferredContext->CSSetShader(m_ComputeShader, nullptr, 0);
    }
    else
    {
        if (!currentPipeline || m_Topology != currentPipeline->m_Topology)
            deferredContext->IASetPrimitiveTopology(m_Topology);

        if (!currentPipeline || m_InputLayout != currentPipeline->m_InputLayout)
            deferredContext->IASetInputLayout(m_InputLayout);

        if (!currentPipeline || m_RasterizerState != currentPipeline->m_RasterizerState)
            deferredContext->RSSetState(m_RasterizerState);

        if (!currentPipeline || m_DepthStencilState != currentPipeline->m_DepthStencilState || m_StencilRef != currentPipeline->m_StencilRef)
            deferredContext->OMSetDepthStencilState(m_DepthStencilState, m_StencilRef);

        if (!currentPipeline || m_BlendState != currentPipeline->m_BlendState || m_SampleMask != currentPipeline->m_SampleMask || memcmp(&m_BlendFactor.x, &currentPipeline->m_BlendFactor.x, sizeof(m_BlendFactor)))
            deferredContext->OMSetBlendState(m_BlendState, &m_BlendFactor.x, m_SampleMask);

        if (!currentPipeline || m_VertexShader != currentPipeline->m_VertexShader)
            deferredContext->VSSetShader(m_VertexShader, nullptr, 0);

        if (!currentPipeline || m_TessControlShader != currentPipeline->m_TessControlShader)
            deferredContext->HSSetShader(m_TessControlShader, nullptr, 0);

        if (!currentPipeline || m_TessEvaluationShader != currentPipeline->m_TessEvaluationShader)
            deferredContext->DSSetShader(m_TessEvaluationShader, nullptr, 0);

        if (!currentPipeline || m_GeometryShader != currentPipeline->m_GeometryShader)
            deferredContext->GSSetShader(m_GeometryShader, nullptr, 0);

        if (!currentPipeline || m_FragmentShader != currentPipeline->m_FragmentShader)
            deferredContext->PSSetShader(m_FragmentShader, nullptr, 0);

        if (m_IsRasterizerDiscarded)
        {
            // no RASTERIZER_DISCARD support in DX11, below is the simplest emulation
            D3D11_RECT rect = { -1, -1, -1, -1 };
            deferredContext->RSSetScissorRects(1, &rect);
        }
    }
}

//================================================================================================================
// NRI
//================================================================================================================

void PipelineD3D11::SetDebugName(const char* name)
{
    SET_D3D_DEBUG_OBJECT_NAME(m_VertexShader, name);
    SET_D3D_DEBUG_OBJECT_NAME(m_TessControlShader, name);
    SET_D3D_DEBUG_OBJECT_NAME(m_TessEvaluationShader, name);
    SET_D3D_DEBUG_OBJECT_NAME(m_GeometryShader, name);
    SET_D3D_DEBUG_OBJECT_NAME(m_FragmentShader, name);
    SET_D3D_DEBUG_OBJECT_NAME(m_ComputeShader, name);
    SET_D3D_DEBUG_OBJECT_NAME(m_InputLayout, name);
    SET_D3D_DEBUG_OBJECT_NAME(m_DepthStencilState, name);
    SET_D3D_DEBUG_OBJECT_NAME(m_BlendState, name);

    for (size_t i = 0; i < m_RasterizerStates.size(); i++)
        SET_D3D_DEBUG_OBJECT_NAME(m_RasterizerStates[i].ptr, name);
}
