#pragma once
#include "Foundation/NonCopyable.h"
#include "D3d12/RootSignature.h"
#include "Utils/GlobalCallbacks.h"
#include "RenderPass.h"

struct PostProcessPassDrawArgs {
	D3D12_CPU_DESCRIPTOR_HANDLE inputSRV;
	dx::GraphicsContext *pGfxCtx;
};

class PostProcessPass : public RenderPass {
public:
	void OnCreate();
	void OnDestroy() override;
	void Draw(const PostProcessPassDrawArgs &args);
private:
	void CreatePipelineState();
	void BuildRenderSettingUI();
private:
	// clang-format off
	SharedPtr<dx::RootSignature>		 _pRootSignature;
	dx::WRL::ComPtr<ID3D12PipelineState> _pPipelineState;
	CallbackHandle						 _buildRenderSettingUIHandle;
	// clang-format on
};