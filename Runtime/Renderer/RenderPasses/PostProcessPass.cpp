#include "PostProcessPass.h"
#include "D3d12/Context.h"
#include "D3d12/Device.h"
#include "D3d12/ShaderCompiler.h"
#include "D3d12/SwapChain.h"
#include "Renderer/GfxDevice.h"
#include "Renderer/RenderSetting.h"
#include "Renderer/RenderUtils/UserMarker.h"
#include "ShaderLoader/ShaderManager.h"
#include "Utils/AssetProjectSetting.h"
#include <imgui.h>

void PostProcessPass::OnCreate() {
    GfxDevice *pGfxDevice = GfxDevice::GetInstance();

    _rootSignature.OnCreate(2, 1);
    _rootSignature.SetStaticSampler(0, dx::GetLinearClampStaticSampler(0));
    _rootSignature.At(0).InitAsConstants(3, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);    // CbSetting
    _rootSignature.At(1).InitAsDescriptorTable(
        {
            CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0),    // gInput
        },
        D3D12_SHADER_VISIBILITY_PIXEL);
    _rootSignature.Generate(pGfxDevice->GetDevice());

    CreatePipelineState();
    _buildRenderSettingUIHandle = GlobalCallbacks::Get().OnBuildRenderSettingGUI.Register(this,
        &PostProcessPass::BuildRenderSettingUI);
}

void PostProcessPass::OnDestroy() {
    _rootSignature.OnDestroy();
    _pPipelineState = nullptr;
}

void PostProcessPass::Draw(const PostProcessPassDrawArgs &args) {
    UserMarker userMarker(args.pGfxCtx, "PostProcessPass");
    Assert(args.width > 0);
    Assert(args.height > 0);

    D3D12_VIEWPORT viewport = {
        0,
        0,
        static_cast<float>(args.width),
        static_cast<float>(args.height),
        0.f,
        1.f,
    };
    D3D12_RECT scissor = {
        0,
        0,
        static_cast<LONG>(args.width),
        static_cast<LONG>(args.height),
    };

    args.pGfxCtx->SetViewport(viewport);
    args.pGfxCtx->SetScissor(scissor);
    args.pGfxCtx->SetGraphicsRootSignature(&_rootSignature);
    args.pGfxCtx->SetPipelineState(_pPipelineState.Get());
    args.pGfxCtx->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    std::vector<dx::DWParam> constants;
    constants.push_back(RenderSetting::Get().GetExposure());
    constants.push_back(RenderSetting::Get().GetGamma());
    constants.push_back(static_cast<int>(RenderSetting::Get().GetToneMapperType()));
    args.pGfxCtx->SetGraphics32Constants(0, constants);
    args.pGfxCtx->SetDynamicViews(1, args.inputSRV);
    args.pGfxCtx->DrawInstanced(3, 1, 0, 0);
}

void PostProcessPass::CreatePipelineState() {
    GfxDevice *pGfxDevice = GfxDevice::GetInstance();
    dx::NativeDevice *device = pGfxDevice->GetDevice()->GetNativeDevice();

    struct PipelineStateStream {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
        CD3DX12_PIPELINE_STATE_STREAM_VS VS;
        CD3DX12_PIPELINE_STATE_STREAM_PS PS;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
    };

    ShaderLoadInfo vsShaderLoadInfo = {};
    vsShaderLoadInfo.sourcePath = AssetProjectSetting::ToAssetPath("Shaders/FullScreenVS.hlsli");
    vsShaderLoadInfo.entryPoint = "VSMain";
    vsShaderLoadInfo.shaderType = dx::ShaderType::eVS;

    ShaderLoadInfo psShaderLoadInfo = {};
    psShaderLoadInfo.sourcePath = AssetProjectSetting::ToAssetPath("Shaders/PostProcessPS.hlsl");
    psShaderLoadInfo.entryPoint = "PSMain";
    psShaderLoadInfo.shaderType = dx::ShaderType::ePS;

    PipelineStateStream pipelineDesc = {};
    pipelineDesc.pRootSignature = _rootSignature.GetRootSignature();
    pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineDesc.VS = ShaderManager::GetInstance()->LoadShaderByteCode(vsShaderLoadInfo);
    pipelineDesc.PS = ShaderManager::GetInstance()->LoadShaderByteCode(psShaderLoadInfo);

    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.NumRenderTargets = 1;
    rtvFormats.RTFormats[0] = pGfxDevice->GetSwapChain()->GetFormat();
    pipelineDesc.RTVFormats = rtvFormats;

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        sizeof(PipelineStateStream),
        &pipelineDesc,
    };
    dx::ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&_pPipelineState)));
}

void PostProcessPass::BuildRenderSettingUI() {
    RenderSetting &renderSetting = RenderSetting::Get();

    const char *toneMapperType[] = {
        "AMDToneMapper",
        "DX11SDK",
        "Reinhard",
        "Uncharted2ToneMap",
        "ACESFilm",
        "None",
    };

    if (!ImGui::TreeNode("PostProcess")) {
        return;
    }

    int toneMapper = static_cast<int>(renderSetting.GetToneMapperType());
    if (ImGui::Combo("ToneMapper", &toneMapper, toneMapperType, std::size(toneMapperType))) {
        renderSetting.SetToneMapperType(static_cast<ToneMapperType>(toneMapper));
    }

    float gamma = renderSetting.GetGamma();
    if (ImGui::DragFloat("Gamma", &gamma, 0.1f, 0.f, 3.f)) {
        renderSetting.SetGamma(gamma);
    }

    float exposure = renderSetting.GetExposure();
    if (ImGui::SliderFloat("Exposure", &exposure, 0.f, 5.f)) {
        renderSetting.SetExposure(exposure);
    }

    ImGui::TreePop();
}
