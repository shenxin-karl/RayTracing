#pragma once
#include "RenderPass.h"
#include "D3d12/D3dStd.h"
#include "D3d12/DescriptorHandle.h"
#include "Foundation/Memory/SharedPtr.hpp"
#include "SceneObject/RayTracingGeometry.h"
#include "Utils/GlobalCallbacks.h"

class RenderView;
class Denoiser;

namespace dx {
struct DispatchRaysDesc;
}

class RayTracingShadowPass : public RenderPass {
public:
    void OnCreate();
    void OnDestroy() override;

    enum GlobalParam { eScene, eRayGenCb, eTable0 };
    enum GlobalParamTable0 {
        eDepthTex,
        eOutputShadowData,
    };

    enum LocalParam {
        eMaterialIndex,
        eVertexBuffer,
        eIndexBuffer,
        eAlbedoTextureList,
        eInstanceMaterial,
    };

    struct DrawArgs {
        const RegionTopLevelAS *pRegionTopLevelAs;
        D3D12_CPU_DESCRIPTOR_HANDLE depthTexSRV;
        const RenderView *pRenderView;
        dx::ComputeContext *pComputeContext;
        Denoiser *pDenoiser = nullptr;
    };
    void GenerateShadowMap(const DrawArgs &args);
    auto GetShadowMaskSRV() const -> D3D12_CPU_DESCRIPTOR_HANDLE;
    void OnResize(const ResolutionInfo &resolution) override;
private:
    void GenerateShadowData(const DrawArgs &args);
    void ShadowDenoise(const DrawArgs &args);
    void CreatePipelineState();
    void BuildShaderRecode(ReadonlyArraySpan<RayTracingGeometry> geometries,
        dx::ComputeContext *pComputeContext,
        dx::DispatchRaysDesc &dispatchRaysDesc) const;
    void BuildRenderSettingUI();
private:
    // clang-format off
	dx::SRV							   _shadowMaskSRV;
	dx::UAV							   _shadowMaskUAV;
    SharedPtr<dx::Texture>             _pShadowMaskTex;

	SharedPtr<dx::Texture>	           _pShadowDataTex;
    dx::UAV                            _shadowDataUAV;

	SharedPtr<dx::RootSignature>       _pGlobalRootSignature;
	SharedPtr<dx::RootSignature>       _pAlphaTestLocalRootSignature;
	dx::WRL::ComPtr<ID3D12StateObject> _pRayTracingPSO;
    CallbackHandle                     _buildRenderSettingUiHandle;
    // clang-format on
};
