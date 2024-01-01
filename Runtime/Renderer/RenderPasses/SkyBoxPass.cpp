#include "SkyBoxPass.h"
#include "D3d12/Context.h"
#include "D3d12/Device.h"
#include "D3d12/RootSignature.h"
#include "D3d12/ShaderCompiler.h"
#include "Renderer/GfxDevice.h"
#include "Renderer/RenderSetting.h"
#include "Renderer/RenderUtils/UserMarker.h"
#include "RenderObject/GPUMeshData.h"
#include "RenderObject/Mesh.h"
#include "RenderObject/VertexSemantic.hpp"
#include "ShaderLoader/ShaderManager.h"
#include "Utils/AssetProjectSetting.h"
#include "Utils/BuildInResource.h"

SkyBoxPass::SkyBoxPass() {
}

SkyBoxPass::~SkyBoxPass() {
}

void SkyBoxPass::OnCreate() {
    RenderPass::OnCreate();

    dx::Device *pDevice = GfxDevice::GetInstance()->GetDevice();
    _pRootSignature = std::make_unique<dx::RootSignature>();
    _pRootSignature->OnCreate(eNumRootParam, 1);
    _pRootSignature->At(eCbSetting).InitAsBufferCBV(0);
    _pRootSignature->At(eCubeMap).InitAsDescriptorTable({
        CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0),
    }, D3D12_SHADER_VISIBILITY_PIXEL);
    _pRootSignature->SetStaticSampler(0, dx::GetLinearClampStaticSampler(0));
    _pRootSignature->Generate(pDevice);

    ShaderLoadInfo shaderLoadInfo;
    shaderLoadInfo.sourcePath = AssetProjectSetting::ToAssetPath("Shaders/SkyBox.hlsl");
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

    GfxDevice *pGfxDevice = GfxDevice::GetInstance();

    PipelineStateStream pipelineDesc = {};
    pipelineDesc.pRootSignature = _pRootSignature->GetRootSignature();
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
    dx::NativeDevice *device = pDevice->GetNativeDevice();
    dx::ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&_pPipelineState)));
}

void SkyBoxPass::OnDestroy() {
    RenderPass::OnDestroy();
    _pRootSignature = nullptr;
    _pPipelineState = nullptr;
}

void SkyBoxPass::Draw(const DrawArgs &drawArgs) {
    dx::GraphicsContext *pGfxCtx = drawArgs.pGfxCtx;
    UserMarker userMarker(pGfxCtx, "SkyBoxPass");

    glm::mat4 matView = drawArgs.matView;
    matView[3][0] = 0.f;
    matView[3][1] = 0.f;
    matView[3][2] = 0.f;

    struct CbSetting {
        glm::mat4x4 matViewProj;
        float       reversedZ;
    };

    CbSetting cbSetting;
    cbSetting.matViewProj = drawArgs.matProj * matView;
    cbSetting.reversedZ = RenderSetting::Get().GetReversedZ() ? 1.f : 0.f;

    pGfxCtx->SetGraphicsRootSignature(_pRootSignature.get());
    pGfxCtx->SetPipelineState(_pPipelineState.Get());

	pGfxCtx->SetGraphicsRootDynamicConstantBuffer(eCbSetting, cbSetting);
    pGfxCtx->SetDynamicViews(eCubeMap, drawArgs.cubeMapSRV);

    std::shared_ptr<Mesh> pSkyBoxCubeMesh = BuildInResource::Get().GetSkyBoxCubeMesh();
    D3D12_VERTEX_BUFFER_VIEW vertexBuffer = pSkyBoxCubeMesh->GetGPUMeshData()->GetVertexBufferView();
    pGfxCtx->SetVertexBuffers(0, vertexBuffer);
    pGfxCtx->DrawInstanced(pSkyBoxCubeMesh->GetVertexCount(), 1, 0, 0);
}
