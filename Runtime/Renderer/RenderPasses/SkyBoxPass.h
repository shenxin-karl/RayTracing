#pragma once
#include "RenderPass.h"
#include "D3d12/D3dStd.h"
#include "Utils/GlobalCallbacks.h"

class RenderView;
class SkyBoxPass : public RenderPass {
public:
	SkyBoxPass();
	~SkyBoxPass() override;
	void OnCreate(DXGI_FORMAT renderTargetFormat);
	void OnDestroy() override;

	// clang-format off
	enum RootParam {
		eCbSetting,
		eCubeMap,
		eNumRootParam,
	};

	struct DrawArgs {
		D3D12_CPU_DESCRIPTOR_HANDLE cubeMapSRV;
		const RenderView		   *pRenderView;
		dx::GraphicsContext		   *pGfxCtx;
	};
	// clang-format on

	void Draw(const DrawArgs &drawArgs);
private:
	void CreatePipelineState(DXGI_FORMAT renderTargetFormat);
private:
	// clang-format off
	std::unique_ptr<dx::RootSignature>		_pRootSignature;
	dx::WRL::ComPtr<ID3D12PipelineState>	_pPipelineState;
	CallbackHandle							_recreatePipelineStateCallbackHandle;

	// clang-format on
};
