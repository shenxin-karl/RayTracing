#pragma once
#include "RenderPass.h"
#include "D3d12/D3dUtils.h"

class SkyBoxPass : public RenderPass {
public:
	SkyBoxPass();
	~SkyBoxPass() override;
	void OnCreate() override;
	void OnDestroy() override;

	// clang-format off
	enum RootParam {
		eCbSetting,
		eCubeMap,
		eNumRootParam,
	};

	struct DrawArgs {
		D3D12_CPU_DESCRIPTOR_HANDLE cubeMapSRV;
		glm::mat4					matView;
		glm::mat4					matProj;
		dx::GraphicsContext		   *pGfxCtx;
	};
	// clang-format on

	void Draw(const DrawArgs &drawArgs);
private:
	// clang-format off
	std::unique_ptr<dx::RootSignature>		_pRootSignature;
	dx::WRL::ComPtr<ID3D12PipelineState>	_pPipelineState;
	// clang-format on
};
