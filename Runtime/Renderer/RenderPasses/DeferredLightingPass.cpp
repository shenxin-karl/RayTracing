#include "DeferredLightingPass.h"
#include "D3d12/Context.h"
#include "D3d12/Device.h"
#include "D3d12/RootSignature.h"
#include "D3d12/ShaderCompiler.h"
#include "Renderer/GfxDevice.h"
#include "ShaderLoader/ShaderManager.h"
#include "Utils/AssetProjectSetting.h"

void DeferredLightingPass::OnCreate() {
	RenderPass::OnCreate();

	GfxDevice *pGfxDevice = GfxDevice::GetInstance();
	_pRootSignature = std::make_unique<dx::RootSignature>();
	_pRootSignature->OnCreate(eNumRootParam);
	_pRootSignature->At(eCbPrePass).InitAsBufferCBV(1);	// b1
	_pRootSignature->At(eCbLighting).InitAsBufferCBV(0);	// b0
	_pRootSignature->At(eTable0).InitAsDescriptorTable({
		// t0 - t3
		CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0),
		// u0
		CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0),
	});
	_pRootSignature->Generate(pGfxDevice->GetDevice());
	_pRootSignature->SetName("DeferredLightingPass::RootSignature");

	struct PipelineDesc {
		CD3DX12_PIPELINE_STATE_STREAM_CS CS;
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE RootSignature;
	};

	ShaderLoadInfo shaderLoadInfo;
    shaderLoadInfo.sourcePath = AssetProjectSetting::ToAssetPath("Shaders/DeferredLightingCS.hlsl");
    shaderLoadInfo.entryPoint = "CSMain";
    shaderLoadInfo.shaderType = dx::ShaderType::eCS;
    shaderLoadInfo.pDefineList = nullptr;
	D3D12_SHADER_BYTECODE csByteCode = ShaderManager::GetInstance()->LoadShaderByteCode(shaderLoadInfo);
	Assert(csByteCode.pShaderBytecode != nullptr);

	PipelineDesc pipelineDesc = {};
	pipelineDesc.CS = csByteCode;
	pipelineDesc.RootSignature = _pRootSignature->GetRootSignature();

	D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        sizeof(pipelineDesc),
        &pipelineDesc,
    };

    dx::NativeDevice *device = pGfxDevice->GetDevice()->GetNativeDevice();
    dx::ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&_pPipelineState)));
}

void DeferredLightingPass::OnDestroy() {
	RenderPass::OnDestroy();
	_pRootSignature->OnDestroy();
	_pRootSignature = nullptr;
	_pPipelineState = nullptr;
}

void DeferredLightingPass::Draw(const DrawArgs &args) {
	dx::ComputeContext *pComputeCtx = args.pComputeCtx;
	pComputeCtx->SetComputeRootSignature(_pRootSignature.get());
	pComputeCtx->SetPipelineState(_pPipelineState.Get());
	pComputeCtx->SetComputeRootConstantBufferView(eCbLighting, args.cbLightingAddress);
	pComputeCtx->SetComputeRootConstantBufferView(eCbPrePass, args.cbPrePassAddress);

	D3D12_CPU_DESCRIPTOR_HANDLE table0Handles[eTable0DescriptorCount];
	table0Handles[eGBuffer0] = args.gBufferSRV[0];
	table0Handles[eGBuffer1] = args.gBufferSRV[1];
	table0Handles[eGBuffer2] = args.gBufferSRV[2];
	table0Handles[eDepthTex] = args.depthStencilSRV;
	table0Handles[eOutput] = args.outputUAV;
	pComputeCtx->SetDynamicViews(eTable0, table0Handles);

	UINT threadX = dx::DivideRoundingUp(args.width, 16);
	UINT threadY = dx::DivideRoundingUp(args.height, 32);
	pComputeCtx->Dispatch(threadX, threadY, 1);
}
