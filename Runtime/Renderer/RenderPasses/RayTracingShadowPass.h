#pragma once
#include "RenderPass.h"
#include "D3d12/D3dStd.h"
#include "D3d12/DescriptorHandle.h"

class RayTracingShadowPass : public RenderPass {
public:
	void OnCreate() override;
	void OnDestroy() override;

	struct DrawArgs {
		D3D12_GPU_VIRTUAL_ADDRESS sceneTopLevelAS;
		dx::ComputeContext *pComputeContext;
	};
	void GenerateShadowMap(const DrawArgs &args);
	auto GetShadowMapSRV() const -> D3D12_CPU_DESCRIPTOR_HANDLE;
private:
	// clang-format off
	dx::SRV							   _shadowMapSRV;
	std::unique_ptr<dx::Texture>	   _pShadowMapTex;
	std::shared_ptr<dx::RootSignature> _pGlobalRootSignature;
	std::shared_ptr<dx::RootSignature> _pAlphaTestLocalRootSignature;
	dx::WRL::ComPtr<ID3D12StateObject> _pRayTracingPSO;
	// clang-format on
};