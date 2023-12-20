#include "PostProcessPass.h"
#include "D3d12/Context.h"
#include "D3d12/Device.h"
#include "D3d12/ShaderCompiler.h"
#include "Renderer/GfxDevice.h"
#include "Renderer/RenderSetting.h"
#include "ShaderLoader/ShaderManager.h"
#include "Utils/AssetProjectSetting.h"

void PostProcessPass::OnCreate() {
	GfxDevice *pGfxDevice = GfxDevice::GetInstance();
	dx::NativeDevice *device = pGfxDevice->GetDevice()->GetNativeDevice();

	_rootSignature.OnCreate(2);
	_rootSignature.At(0).InitAsConstants(3, 0);		// CbSetting
	_rootSignature.At(1).InitAsDescriptorTable({
		CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0),	// gInput
		CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0),	// gOutput
	});
	_rootSignature.Generate(pGfxDevice->GetDevice());

	ShaderLoadInfo shaderLoadInfo = {};
	shaderLoadInfo.sourcePath = AssetProjectSetting::ToAssetPath("Shaders/PostProcess.hlsli");
	shaderLoadInfo.entryPoint = "CSMain";
	shaderLoadInfo.shaderType = dx::ShaderType::eCS;

	D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
	desc.pRootSignature = _rootSignature.GetRootSignature();
	desc.CS = ShaderManager::GetInstance()->LoadShaderByteCode(shaderLoadInfo);
	dx::ThrowIfFailed(device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&_pPipelineState)));
}

void PostProcessPass::OnDestroy() {
	_rootSignature.OnDestroy();
	_pPipelineState = nullptr;
}

void PostProcessPass::Draw(const PostProcessPassDrawArgs &args) {
	Assert(args.width > 0);
	Assert(args.height > 0);


	args.pComputeCtx->SetComputeRootSignature(&_rootSignature);
	args.pComputeCtx->SetPipelineState(_pPipelineState.Get());

	std::vector<dx::DWParam> constants;
	constants.push_back(RenderSetting::Get().GetExposure());
	constants.push_back(static_cast<int>(RenderSetting::Get().GetToneMapperType()));
	args.pComputeCtx->SetCompute32Constants(0, constants);
	args.pComputeCtx->SetDynamicViews(1, args.inputSRV, 0);
	args.pComputeCtx->SetDynamicViews(1, args.outputUAV, 1);

	UINT threadX = dx::AlignUp(args.width, 8);
	UINT threadY = dx::AlignUp(args.height, 8);
	args.pComputeCtx->Dispatch(threadX, threadY, 1);
}
