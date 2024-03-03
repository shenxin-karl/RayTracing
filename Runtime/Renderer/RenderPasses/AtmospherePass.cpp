#include "AtmospherePass.h"
#include "D3d12/Device.h"
#include "D3d12/ShaderCompiler.h"
#include "Renderer/GfxDevice.h"
#include "ShaderLoader/ShaderManager.h"
#include "Utils/AssetProjectSetting.h"
#include "Foundation/ColorUtil.hpp"

constexpr size_t kSkyBoxLutWidth = 256;
constexpr size_t kSkyBoxLutHeight = 128;

AtmospherePass::AtmospherePass() {
    parameter.seaLevel = 0.f;
    parameter.planetRadius = 6360000.0f;
    parameter.atmosphereHeight = 60000.0f;
    parameter.sunLightColor = Colors::White;
    parameter.sunDiskAngle = 9.f;
    parameter.sunLightIntensity = 1.f;
    parameter.rayleighScatteringScalarHeight = 8000.f;
    parameter.mieAnisotropy = 0.8f;
    parameter.mieScatteringScalarHeight = 1200.0f;
    parameter.ozoneLevelCenterHeight = 25000.f;
    parameter.ozoneLevelWidth = 15000.f;
}

void AtmospherePass::OnCreate() {
    CreateSkyboxLutObject();
}

void AtmospherePass::OnDestroy() {
    RenderPass::OnDestroy();
    _pSkyBoxLutTex.Release();
    _pSkyBoxLutRS.Release();
    _pSkyBoxLutPSO.Reset();
    _skyboxLutSRV.Release();
    _skyboxLutUAV.Release();
}

void AtmospherePass::GenerateLut(AtmospherePassArgs args) {
    GenerateSkyBoxLut(args);
}

void AtmospherePass::CreateSkyboxLutObject() {
    GfxDevice *pGfxDevice = GfxDevice::GetInstance();
    D3D12_RESOURCE_DESC skyBoxLutDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R11G11B10_FLOAT,
        kSkyBoxLutWidth,
        kSkyBoxLutHeight,
        6);
    skyBoxLutDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    _pSkyBoxLutTex = dx::Texture::Create(pGfxDevice->GetDevice(), skyBoxLutDesc, D3D12_RESOURCE_STATE_COMMON);

    _pSkyBoxLutRS = dx::RootSignature::Create(3);
    _pSkyBoxLutRS->At(0).InitAsBufferCBV(0);
    _pSkyBoxLutRS->At(1).InitAsBufferCBV(1);
    _pSkyBoxLutRS->At(2).InitAsDescriptorTable({
        {D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0},
    });
    _pSkyBoxLutRS->Generate(pGfxDevice->GetDevice());

    dx::NativeDevice *device = pGfxDevice->GetDevice()->GetNativeDevice();
    ShaderLoadInfo shaderLoadInfo = {};
    shaderLoadInfo.sourcePath = AssetProjectSetting::ToAssetPath("Shaders/Atmosphere/GenerateSkyboxLutCS.hlsl"),
    shaderLoadInfo.entryPoint = "CSMain";
    shaderLoadInfo.shaderType = dx::ShaderType::eCS;

    D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineStateDesc = {};
    pipelineStateDesc.pRootSignature = _pSkyBoxLutRS->GetRootSignature();
    pipelineStateDesc.CS = ShaderManager::GetInstance()->LoadShaderByteCode(shaderLoadInfo);
    device->CreateComputePipelineState(&pipelineStateDesc, IID_PPV_ARGS(&_pSkyBoxLutPSO));

    _skyboxLutSRV = pGfxDevice->GetDevice()->AllocDescriptor<dx::SRV>(1);
    D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceView = {};
    shaderResourceView.Format = skyBoxLutDesc.Format;
    shaderResourceView.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    shaderResourceView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    shaderResourceView.TextureCube.MostDetailedMip = 0;
    shaderResourceView.TextureCube.MipLevels = 1;
    shaderResourceView.TextureCube.ResourceMinLODClamp = 0;
    device->CreateShaderResourceView(_pSkyBoxLutTex->GetResource(), &shaderResourceView, _skyboxLutSRV.GetCpuHandle());

    _skyboxLutUAV = pGfxDevice->GetDevice()->AllocDescriptor<dx::UAV>(1);
    device->CreateUnorderedAccessView(_pSkyBoxLutTex->GetResource(), nullptr, nullptr, _skyboxLutUAV.GetCpuHandle());
}

void AtmospherePass::GenerateSkyBoxLut(const AtmospherePassArgs &args) {
    dx::ComputeContext *pComputeCtx = args.pComputeCtx;
    pComputeCtx->SetComputeRootSignature(_pSkyBoxLutRS.Get());
    pComputeCtx->SetPipelineState(_pSkyBoxLutPSO.Get());

    struct CBSetting {
        glm::vec3 lightDir;
        float cameraPosY;
    };

    CBSetting setting;
    setting.lightDir = args.pRenderView->GetCBLighting().directionalLight.direction;
    setting.cameraPosY = args.pRenderView->GetCBPrePass().cameraPos.y;
    pComputeCtx->SetComputeRootDynamicConstantBuffer(0, setting);
    pComputeCtx->SetComputeRootDynamicConstantBuffer(1, parameter);
    pComputeCtx->SetDynamicViews(2, 1, _skyboxLutUAV);

    uint groupX = dx::DivideRoundingUp(kSkyBoxLutWidth, 8);
    uint groupY = dx::DivideRoundingUp(kSkyBoxLutHeight, 6);
    pComputeCtx->Dispatch(groupX, groupY, 6);
}
