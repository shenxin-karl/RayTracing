#pragma once
#include "RenderPass.h"
#include "D3d12/D3dStd.h"
#include "D3d12/DescriptorHandle.h"

class RayTracingShadowPass : public RenderPass {
public:
	void OnCreate() override;
	void OnDestroy() override;

	enum GlobalParam {
		eScene,
		eRayGenCb,
		eTable0
	};
	enum GlobalParamTable0 {
		eDepthTex,
		eOutputTex,
	};

	enum LocalParam {
		eMaterialIndex,
		eVertexBuffer,
		eIndexBuffer,
		eAlbedoTextureList,
		eInstanceMaterial,
	};

	struct DrawArgs {
		D3D12_GPU_VIRTUAL_ADDRESS sceneTopLevelAS;
		dx::ComputeContext *pComputeContext;
	};
	void GenerateShadowMap(const DrawArgs &args);
	auto GetShadowMapSRV() const -> D3D12_CPU_DESCRIPTOR_HANDLE;
	void OnResize(size_t width, size_t height);
private:
	// clang-format off
	dx::SRV							   _shadowMapSRV;
	dx::UAV							   _shadowMapUAV;
	std::unique_ptr<dx::Texture>	   _pShadowMapTex;
	std::shared_ptr<dx::RootSignature> _pGlobalRootSignature;
	std::shared_ptr<dx::RootSignature> _pAlphaTestLocalRootSignature;
	dx::WRL::ComPtr<ID3D12StateObject> _pRayTracingPSO;
	// clang-format on
};