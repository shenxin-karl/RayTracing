#include "RayTracingShadowPass.h"

void RayTracingShadowPass::OnCreate() {
	RenderPass::OnCreate();
}

void RayTracingShadowPass::OnDestroy() {
	RenderPass::OnDestroy();
}

void RayTracingShadowPass::GenerateShadowMap(const DrawArgs &args) {
}

auto RayTracingShadowPass::GetShadowMapSRV() const -> D3D12_CPU_DESCRIPTOR_HANDLE {
	return _shadowMapSRV.GetCpuHandle();
}
