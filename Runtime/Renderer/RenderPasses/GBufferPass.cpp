#include "GBufferPass.h"
#include "D3d12/Context.h"
#include "D3d12/D3dUtils.h"
#include "D3d12/Device.h"
#include "Foundation/ColorUtil.hpp"
#include "Renderer/GfxDevice.h"
#include "Renderer/RenderSetting.h"
#include "RenderObject/Material.h"
#include "RenderObject/Mesh.h"
#include "RenderObject/RenderObject.h"
#include "RenderObject/VertexSemantic.hpp"
#include "ShaderLoader/ShaderManager.h"
#include "Utils/AssetProjectSetting.h"

GBufferPass::GBufferPass() : _width(0), _height(0) {
}

void GBufferPass::OnCreate() {
    GfxDevice *pDevice = GfxDevice::GetInstance();
    _gBufferSRV = pDevice->GetDevice()->AllocDescriptor<dx::SRV>(3);
    _gBufferRTV = pDevice->GetDevice()->AllocDescriptor<dx::RTV>(3);

    _pRootSignature = std::make_unique<dx::RootSignature>();
    _pRootSignature->OnCreate(eMaxNumRootParam, 6);
    _pRootSignature->At(eCbPrePass).InitAsBufferCBV(0);
    _pRootSignature->At(eCbPreObject).InitAsBufferCBV(1);
    _pRootSignature->At(eCbMaterial).InitAsBufferCBV(2);

    // eTextureList enable bindless
    CD3DX12_DESCRIPTOR_RANGE1 range = {
        D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
        static_cast<UINT>(-1),
        0,
        0,
        D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE,
    };
    _pRootSignature->At(eTextureList).InitAsDescriptorTable({range});

    D3D12_STATIC_SAMPLER_DESC samplers[6] = {
        dx::GetPointWrapStaticSampler(0),
        dx::GetPointClampStaticSampler(1),
        dx::GetLinearWrapStaticSampler(2),
        dx::GetLinearClampStaticSampler(3),
        dx::GetAnisotropicWrapStaticSampler(4),
        dx::GetAnisotropicClampStaticSampler(5),
    };
    _pRootSignature->SetStaticSamplers(samplers);
    _pRootSignature->Generate(GfxDevice::GetInstance()->GetDevice());

    _recreatePipelineStateCallbackHandle = GlobalCallbacks::Get().onRecreatePipelineState.Register([&]() {
	    _pipelineStateMap.clear();
    });
}

void GBufferPass::OnDestroy() {
    _gBuffer0.OnDestroy();
    _gBuffer1.OnDestroy();
    _gBuffer2.OnDestroy();
    _gBufferRTV.Release();
    _gBufferSRV.Release();
    _pRootSignature->OnDestroy();
    _pipelineStateMap.clear();
    _recreatePipelineStateCallbackHandle.Release();
}

void GBufferPass::OnResize(size_t width, size_t height) {
    GfxDevice *pDevice = GfxDevice::GetInstance();

    D3D12_CLEAR_VALUE clearValue;
    clearValue.Color[0] = 0.f;
    clearValue.Color[1] = 0.f;
    clearValue.Color[2] = 0.f;
    clearValue.Color[3] = 1.f;

    _gBuffer0.OnDestroy();
    D3D12_RESOURCE_DESC gBuffer0Desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height);
    gBuffer0Desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    _gBuffer0.OnCreate(pDevice->GetDevice(), gBuffer0Desc, D3D12_RESOURCE_STATE_COMMON, &clearValue);

    _gBuffer1.OnDestroy();
    D3D12_RESOURCE_DESC gBuffer1Desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_FLOAT, width, height);
    gBuffer0Desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    clearValue.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    _gBuffer1.OnCreate(pDevice->GetDevice(), gBuffer1Desc, D3D12_RESOURCE_STATE_COMMON, &clearValue);

    _gBuffer2.OnDestroy();
    D3D12_RESOURCE_DESC gBuffer2Desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R11G11B10_FLOAT, width, height);
    gBuffer2Desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    clearValue.Format = DXGI_FORMAT_R11G11B10_FLOAT;
    _gBuffer2.OnCreate(pDevice->GetDevice(), gBuffer2Desc, D3D12_RESOURCE_STATE_COMMON, &clearValue);

    auto CreateView = [=](dx::Texture &texture, D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CPU_DESCRIPTOR_HANDLE srv) {
        dx::NativeDevice *device = pDevice->GetDevice()->GetNativeDevice();
        D3D12_RENDER_TARGET_VIEW_DESC rtvViewDesc = {};
        rtvViewDesc.Format = texture.GetFormat();
        rtvViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        rtvViewDesc.Texture2D.MipSlice = 0;
        rtvViewDesc.Texture2D.PlaneSlice = 0;
        device->CreateRenderTargetView(texture.GetResource(), &rtvViewDesc, rtv);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvViewDesc = {};
        srvViewDesc.Format = texture.GetFormat();
        srvViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvViewDesc.Texture2D.MostDetailedMip = 0;
        srvViewDesc.Texture2D.MipLevels = 1;
        srvViewDesc.Texture2D.PlaneSlice = 0;
        srvViewDesc.Texture2D.ResourceMinLODClamp = 0.f;
        device->CreateShaderResourceView(texture.GetResource(), &srvViewDesc, srv);
    };

    CreateView(_gBuffer0, _gBufferRTV[0], _gBufferSRV[0]);
    CreateView(_gBuffer1, _gBufferRTV[1], _gBufferSRV[1]);
    CreateView(_gBuffer2, _gBufferRTV[2], _gBufferSRV[2]);
}

auto GBufferPass::GetGBufferSRV(size_t index) const -> D3D12_CPU_DESCRIPTOR_HANDLE {
    Assert(index < 3);
    return _gBufferSRV[index];
}

void GBufferPass::PreDraw(const DrawArgs &args) {
    D3D12_RECT scissor = {0, 0, static_cast<LONG>(_width), static_cast<LONG>(_height)};
    D3D12_VIEWPORT viewport = {
        0.f,
        0.f,
        static_cast<float>(_width),
        static_cast<float>(_height),
        0.f,
        1.f,
    };
    args.pGfxCtx->SetViewport(viewport);
    args.pGfxCtx->SetScissor(scissor);

    auto TranslationAndClearRT = [&](ID3D12Resource *pResource, D3D12_CPU_DESCRIPTOR_HANDLE rtv, glm::vec4 color) {
        args.pGfxCtx->Transition(pResource, D3D12_RESOURCE_STATE_RENDER_TARGET);
        args.pGfxCtx->ClearRenderTargetView(rtv, color);
    };

    TranslationAndClearRT(_gBuffer0.GetResource(), _gBufferRTV[0], Colors::White);
    TranslationAndClearRT(_gBuffer1.GetResource(), _gBufferRTV[1], Colors::White);
    TranslationAndClearRT(_gBuffer2.GetResource(), _gBufferRTV[2], Colors::White);

    args.pGfxCtx->Transition(args.pDepthBufferResource, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    args.pGfxCtx->ClearDepthStencilView(args.depthBufferDSV,
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
        RenderSetting::Get().GetDepthClearValue(),
        0);
}

void GBufferPass::DrawBatch(const std::vector<RenderObject *> &batchList, const DrawArgs &args) {
    DrawBatchList(batchList, [&](std::span<RenderObject *const> batch) { DrawBatchInternal(batch, args); });
}

void GBufferPass::PostDraw(const DrawArgs &args) {
}

void GBufferPass::DrawBatchInternal(std::span<RenderObject *const> batch, const DrawArgs &args) {

}

auto GBufferPass::GetPipelineState(RenderObject *pRenderObject) -> ID3D12PipelineState * {
    Material *pMaterial = pRenderObject->pMaterial;
    auto iter = _pipelineStateMap.find(pMaterial->GetPipelineID());
    if (iter != _pipelineStateMap.end()) {
        return iter->second.Get();
    }

    ShaderLoadInfo shaderLoadInfo;
    shaderLoadInfo.sourcePath = AssetProjectSetting::ToAssetPath("Material.hlsl");
    shaderLoadInfo.entryPoint = "VSMain";
    shaderLoadInfo.shaderType = dx::ShaderType::eVS;
    shaderLoadInfo.pDefineList = &pMaterial->_defineList;
    D3D12_SHADER_BYTECODE vsByteCode = ShaderManager::GetInstance()->LoadShaderByteCode(shaderLoadInfo);
    Assert(vsByteCode.pShaderBytecode != nullptr);

    shaderLoadInfo.entryPoint = "GBufferPSMain";
    shaderLoadInfo.shaderType = dx::ShaderType::ePS;
    D3D12_SHADER_BYTECODE psByteCode = ShaderManager::GetInstance()->LoadShaderByteCode(shaderLoadInfo);

    struct PipelineStateStream {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
        CD3DX12_PIPELINE_STATE_STREAM_VS VS;
        CD3DX12_PIPELINE_STATE_STREAM_PS PS;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL DepthStencil;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
    };

    GfxDevice *pGfxDevice = GfxDevice::GetInstance();

    PipelineStateStream pipelineDesc = {};
    pipelineDesc.pRootSignature = _pRootSignature->GetRootSignature();
    pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineDesc.VS = vsByteCode;
    pipelineDesc.PS = psByteCode;
    pipelineDesc.DSVFormat = pGfxDevice->GetDepthStencilFormat();

    CD3DX12_DEPTH_STENCIL_DESC depthStencil(D3D12_DEFAULT);
    depthStencil.DepthFunc = RenderSetting::Get().GetDepthFunc();
    pipelineDesc.DepthStencil = depthStencil;

    SemanticMask meshSemanticMask = pRenderObject->pMesh->GetSemanticMask();
    SemanticMask pipelineSemanticMask = pMaterial->_pipelineSemanticMask;
    auto inputLayouts = SemanticMaskToVertexInputElements(meshSemanticMask, pipelineSemanticMask);
    pipelineDesc.InputLayout = D3D12_INPUT_LAYOUT_DESC{
        inputLayouts.data(),
        static_cast<UINT>(inputLayouts.size()),
    };

    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.NumRenderTargets = 3;
    rtvFormats.RTFormats[0] = _gBuffer0.GetFormat();
    rtvFormats.RTFormats[1] = _gBuffer1.GetFormat();
    rtvFormats.RTFormats[2] = _gBuffer2.GetFormat();
    pipelineDesc.RTVFormats = rtvFormats;

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        sizeof(PipelineStateStream),
        &pipelineDesc,
    };

    dx::WRL::ComPtr<ID3D12PipelineState> pPipelineState;
    dx::NativeDevice *device = pGfxDevice->GetDevice()->GetNativeDevice();
    dx::ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&pPipelineState)));

    _pipelineStateMap[pMaterial->GetPipelineID()] = pPipelineState;
    return pPipelineState.Get();
}
