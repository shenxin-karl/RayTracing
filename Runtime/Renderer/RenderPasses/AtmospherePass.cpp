#include "AtmospherePass.h"
#include "D3d12/Device.h"
#include "D3d12/ShaderCompiler.h"
#include "Renderer/GfxDevice.h"
#include "ShaderLoader/ShaderManager.h"
#include "Utils/AssetProjectSetting.h"
#include "Foundation/ColorUtil.hpp"
#include "Renderer/RenderUtils/RenderSetting.h"
#include "Renderer/RenderUtils/UserMarker.h"
#include "RenderObject/GPUMeshData.h"
#include "RenderObject/Mesh.h"
#include "RenderObject/VertexSemantic.hpp"
#include "Utils/BuildInResource.h"

constexpr size_t kSkyBoxLutWidth = 256;
constexpr size_t kSkyBoxLutHeight = 128;

AtmospherePass::AtmospherePass() {
    parameter.seaLevel = 0.f;
    parameter.planetRadius = 6360000.0f;
    parameter.atmosphereHeight = 60000.0f;
    parameter.sunLightColor = Colors::White;
    parameter.sunDiskAngle = 9.f;
    parameter.sunLightIntensity = 31.4f;
    parameter.rayleighScatteringScalarHeight = 8000.f;
    parameter.mieAnisotropy = 0.8f;
    parameter.mieScatteringScalarHeight = 1200.0f;
    parameter.ozoneLevelCenterHeight = 25000.f;
    parameter.ozoneLevelWidth = 15000.f;
}

void AtmospherePass::OnCreate() {
    CreateSkyboxLutObjects();
    CreateSkyboxObjects();
}

void AtmospherePass::OnDestroy() {
    RenderPass::OnDestroy();
    // SkyBoxLut member
    _pSkyBoxLutTex.Release();
    _pSkyBoxLutRS.Release();
    _pSkyBoxLutPSO.Reset();
    _skyboxLutSRV.Release();
    _skyboxLutUAV.Release();

    // SkyBox member
    _pSkyBoxRS.Release();
    _pSkyBoxPSO.Reset();
}

void AtmospherePass::GenerateLut(AtmospherePassArgs args) {
    UserMarker userMarker(args.pComputeCtx, "AtmospherePass");
    GenerateSkyBoxLut(args);
}

void AtmospherePass::DrawSkyBox(SkyboxArgs args) {
    UserMarker userMarker(args.pGraphicsContext, "AtmospherePass::Draw SkyBox");
    dx::GraphicsContext *pGfxCtx = args.pGraphicsContext;

    const auto &cbPrePass = args.pRenderView->GetCBPrePass();
    glm::mat4 matView = cbPrePass.matView;
    matView[3][0] = 0.f;
    matView[3][1] = 0.f;
    matView[3][2] = 0.f;

    struct CbSetting {
        glm::mat4x4 matViewProj;
        float reversedZ;
    };

    CbSetting cbSetting;
    cbSetting.matViewProj = cbPrePass.matJitteredProj * matView;
    cbSetting.reversedZ = RenderSetting::Get().GetReversedZ() ? 1.f : 0.f;

    pGfxCtx->SetGraphicsRootSignature(_pSkyBoxRS.Get());
    pGfxCtx->SetPipelineState(_pSkyBoxPSO.Get());
    pGfxCtx->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    pGfxCtx->SetGraphicsRootDynamicConstantBuffer(0, cbSetting);
    pGfxCtx->SetDynamicViews(1, _skyboxLutSRV.GetCpuHandle());

    std::shared_ptr<Mesh> pSkyBoxCubeMesh = BuildInResource::Get().GetSkyBoxCubeMesh();
    D3D12_VERTEX_BUFFER_VIEW vertexBuffer = pSkyBoxCubeMesh->GetGPUMeshData()->GetVertexBufferView();
    pGfxCtx->SetVertexBuffers(0, vertexBuffer);
    pGfxCtx->DrawInstanced(pSkyBoxCubeMesh->GetVertexCount(), 1, 0, 0);
}

void AtmospherePass::CreateSkyboxLutObjects() {
    GfxDevice *pGfxDevice = GfxDevice::GetInstance();
    D3D12_RESOURCE_DESC skyBoxLutDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R11G11B10_FLOAT,
        kSkyBoxLutWidth,
        kSkyBoxLutHeight);
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
    device->CreateShaderResourceView(_pSkyBoxLutTex->GetResource(), nullptr, _skyboxLutSRV.GetCpuHandle());

    _skyboxLutUAV = pGfxDevice->GetDevice()->AllocDescriptor<dx::UAV>(1);
    device->CreateUnorderedAccessView(_pSkyBoxLutTex->GetResource(), nullptr, nullptr, _skyboxLutUAV.GetCpuHandle());
}

void AtmospherePass::CreateSkyboxObjects() {
    GfxDevice *pGfxDevice = GfxDevice::GetInstance();
    _pSkyBoxRS = dx::RootSignature::Create(2, 1);
    _pSkyBoxRS->At(0).InitAsBufferCBV(0);
    _pSkyBoxRS->At(1).InitAsDescriptorTable({{D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0}});
    _pSkyBoxRS->SetStaticSampler(0, dx::GetLinearClampStaticSampler(0));
    _pSkyBoxRS->Generate(pGfxDevice->GetDevice());

    ShaderLoadInfo shaderLoadInfo;
    shaderLoadInfo.sourcePath = AssetProjectSetting::ToAssetPath("Shaders/Atmosphere/Skybox.hlsl");
    shaderLoadInfo.entryPoint = "VSMain";
    shaderLoadInfo.shaderType = dx::ShaderType::eVS;
    shaderLoadInfo.pDefineList = nullptr;
    D3D12_SHADER_BYTECODE vsByteCode = ShaderManager::GetInstance()->LoadShaderByteCode(shaderLoadInfo);
    Assert(vsByteCode.pShaderBytecode != nullptr);

    shaderLoadInfo.entryPoint = "PSMain";
    shaderLoadInfo.shaderType = dx::ShaderType::ePS;
    D3D12_SHADER_BYTECODE psByteCode = ShaderManager::GetInstance()->LoadShaderByteCode(shaderLoadInfo);

    struct PipelineStateStream {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
        CD3DX12_PIPELINE_STATE_STREAM_VS VS;
        CD3DX12_PIPELINE_STATE_STREAM_PS PS;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL DepthStencil;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DepthStencilFormat;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
    };

    PipelineStateStream pipelineDesc = {};
    pipelineDesc.pRootSignature = _pSkyBoxRS->GetRootSignature();
    pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineDesc.VS = vsByteCode;
    pipelineDesc.PS = psByteCode;

    CD3DX12_DEPTH_STENCIL_DESC depthStencil(D3D12_DEFAULT);
    depthStencil.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

    if (RenderSetting::Get().GetReversedZ()) {
        depthStencil.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    } else {
        depthStencil.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    }

    pipelineDesc.DepthStencil = depthStencil;

    auto inputLayouts = SemanticMaskToVertexInputElements(SemanticMask::eVertex);
    pipelineDesc.InputLayout = D3D12_INPUT_LAYOUT_DESC{
        inputLayouts.data(),
        static_cast<UINT>(inputLayouts.size()),
    };

    pipelineDesc.DepthStencilFormat = pGfxDevice->GetDepthStencilFormat();

    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.RTFormats[0] = pGfxDevice->GetRenderTargetFormat();
    rtvFormats.NumRenderTargets = 1;
    pipelineDesc.RTVFormats = rtvFormats;

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        sizeof(PipelineStateStream),
        &pipelineDesc,
    };

    dx::WRL::ComPtr<ID3D12PipelineState> pPipelineState;
    dx::NativeDevice *device = pGfxDevice->GetDevice()->GetNativeDevice();
    dx::ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&_pSkyBoxPSO)));
}

void AtmospherePass::GenerateSkyBoxLut(const AtmospherePassArgs &args) {
    UserMarker userMarker(args.pComputeCtx, "GenerateSkyBoxLut");

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
    pComputeCtx->Dispatch(groupX, groupY, 1);

    pComputeCtx->Transition(_pSkyBoxLutTex->GetResource(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}
