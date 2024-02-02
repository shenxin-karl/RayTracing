#pragma once
#include <memory>
#include "RenderPass.h"
#include "D3d12/D3dStd.h"
#include "Renderer/RenderUtils/RenderView.h"

class DeferredLightingPass : public RenderPass {
public:
    void OnCreate();
    void OnDestroy() override;

    // clang-format off
	enum RootParam {
		eCbPrePass,
		eCbLighting,
		eTable0,
		eNumRootParam
	};

	enum Table0Param {
		eGBuffer0,
		eGBuffer1,
		eGBuffer2,
		eDepthTex,
		eShadowMask,
		eOutput,
		eTable0DescriptorCount,
	};

	struct DispatchArgs {
		const RenderView		   *pRenderView;
		D3D12_GPU_VIRTUAL_ADDRESS   cbPrePassAddress;
		D3D12_GPU_VIRTUAL_ADDRESS	cbLightingAddress;
		D3D12_CPU_DESCRIPTOR_HANDLE gBufferSRV[3];
		D3D12_CPU_DESCRIPTOR_HANDLE depthStencilSRV;
		D3D12_CPU_DESCRIPTOR_HANDLE shadowMaskSRV;
		D3D12_CPU_DESCRIPTOR_HANDLE outputUAV;
		dx::ComputeContext		   *pComputeCtx;
	};
    // clang-format on
	void Dispatch(const DispatchArgs &args);
private:
    // clang-format off
	std::unique_ptr<dx::RootSignature>	 _pRootSignature;
	dx::WRL::ComPtr<ID3D12PipelineState> _pPipelineState;
    // clang-format on
};
