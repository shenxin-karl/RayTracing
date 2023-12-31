#pragma once
#include "Foundation/NonCopyable.h"
#include "D3d12/RootSignature.h"
#include "Utils/GlobalCallbacks.h"

struct PostProcessPassDrawArgs {
	size_t width;
	size_t height;
	D3D12_CPU_DESCRIPTOR_HANDLE inputSRV;
	dx::GraphicsContext *pGfxCtx;
};

class PostProcessPass : public NonCopyable {
public:
	void OnCreate();
	void OnDestroy();
	void Draw(const PostProcessPassDrawArgs &args);
private:
	void CreatePipelineState();
	void BuildRenderSettingUI();
private:
	// clang-format off
	dx::RootSignature					 _rootSignature;
	dx::WRL::ComPtr<ID3D12PipelineState> _pPipelineState;
	CallbackHandle						 _buildRenderSettingUIHandle;
	// clang-format on
};