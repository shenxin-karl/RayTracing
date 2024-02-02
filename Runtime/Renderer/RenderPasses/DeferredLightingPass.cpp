#include "DeferredLightingPass.h"
#include "D3d12/Context.h"
#include "D3d12/Device.h"
#include "D3d12/RootSignature.h"
#include "D3d12/ShaderCompiler.h"
#include "Renderer/GfxDevice.h"
#include "ShaderLoader/ShaderManager.h"
#include "Utils/AssetProjectSetting.h"
#include "Renderer/RenderUtils/UserMarker.h"

void DeferredLightingPass::OnCreate() {
	GfxDevice *pGfxDevice = GfxDevice::GetInstance();
	_pRootSignature = std::make_unique<dx::RootSignature>();
	_pRootSignature->OnCreate(eNumRootParam);
	_pRootSignature->At(eCbPrePass).InitAsBufferCBV(1);	// b1
	_pRootSignature->At(eCbLighting).InitAsBufferCBV(0);	// b0
	_pRootSignature->At(eTable0).InitAsDescriptorTable({
		// t0 - t4
		CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 0),
		// u0
		CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0),
	});
	_pRootSignature->Generate(pGfxDevice->GetDevice());
	_pRootSignature->SetName("DeferredLightingPass::RootSignature");

	struct PipelineDesc {
		CD3DX12_PIPELINE_STATE_STREAM_CS CS;
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE RootSignature;
	};

	dx::DefineList defineList;
	defineList["THREAD_WRAP_SIZE"] = pGfxDevice->GetDevice()->GetWorkGroupWarpSize();

	ShaderLoadInfo shaderLoadInfo;
    shaderLoadInfo.sourcePath = AssetProjectSetting::ToAssetPath("Shaders/DeferredLightingCS.hlsl");
    shaderLoadInfo.entryPoint = "CSMain";
    shaderLoadInfo.shaderType = dx::ShaderType::eCS;
    shaderLoadInfo.pDefineList = &defineList;
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

void DeferredLightingPass::Dispatch(const DispatchArgs &args) {
	dx::ComputeContext *pComputeCtx = args.pComputeCtx;
	UserMarker marker(pComputeCtx, "DeferredLightingPass");

	pComputeCtx->SetComputeRootSignature(_pRootSignature.get());
	pComputeCtx->SetPipelineState(_pPipelineState.Get());
	pComputeCtx->SetComputeRootConstantBufferView(eCbLighting, args.cbLightingAddress);
	pComputeCtx->SetComputeRootConstantBufferView(eCbPrePass, args.cbPrePassAddress);

	D3D12_CPU_DESCRIPTOR_HANDLE table0Handles[eTable0DescriptorCount];
	table0Handles[eGBuffer0] = args.gBufferSRV[0];
	table0Handles[eGBuffer1] = args.gBufferSRV[1];
	table0Handles[eGBuffer2] = args.gBufferSRV[2];
	table0Handles[eDepthTex] = args.depthStencilSRV;
	table0Handles[eShadowMask] = args.shadowMaskSRV;
	table0Handles[eOutput] = args.outputUAV;
	pComputeCtx->SetDynamicViews(eTable0, table0Handles);

	dx::Device *pDevice = GfxDevice::GetInstance()->GetDevice();
	const RenderView *pRenderView = args.pRenderView;
	glm::vec2 renderSize = pRenderView->GetCBPrePass().renderSize;
	UINT threadX = dx::DivideRoundingUp(static_cast<size_t>(renderSize.x),  pDevice->GetWorkGroupWarpSize());
	UINT threadY = dx::DivideRoundingUp(static_cast<size_t>(renderSize.y), 16);
	pComputeCtx->Dispatch(threadX, threadY, 1);
}
