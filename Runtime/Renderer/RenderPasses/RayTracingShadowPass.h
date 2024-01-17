#pragma once
#include "RenderPass.h"
#include "D3d12/D3dStd.h"
#include "D3d12/DescriptorHandle.h"
#include "SceneObject/RayTracingGeometry.h"
#include "Utils/GlobalCallbacks.h"

namespace dx {
struct DispatchRaysDesc;
}

class RayTracingShadowPass : public RenderPass {
public:
    void OnCreate() override;
    void OnDestroy() override;

    enum GlobalParam { eScene, eRayGenCb, eTable0 };
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
        ReadonlyArraySpan<RayTracingGeometry> geometries;
        D3D12_CPU_DESCRIPTOR_HANDLE depthTexSRV;
        glm::vec3 lightDirection;
        glm::vec4 zBufferParams;
        glm::mat4x4 matInvViewProj;
        dx::ComputeContext *pComputeContext;
    };
    void GenerateShadowMap(const DrawArgs &args);
    auto GetShadowMaskSRV() const -> D3D12_CPU_DESCRIPTOR_HANDLE;
    void OnResize(size_t width, size_t height);
private:
    void CreatePipelineState();
    void BuildShaderRecode(ReadonlyArraySpan<RayTracingGeometry> geometries,
        dx::ComputeContext *pComputeContext,
        dx::DispatchRaysDesc &dispatchRaysDesc) const;
    void BuildRenderSettingUI();
private:
    // clang-format off
	dx::SRV							   _shadowMaskSRV;
	dx::UAV							   _shadowMaskUAV;
    std::unique_ptr<dx::Texture>       _pShadowMaskTex;

	std::unique_ptr<dx::Texture>	   _pShadowDataTex;
    dx::UAV                            _shadowDataUAV;

	std::shared_ptr<dx::RootSignature> _pGlobalRootSignature;
	std::shared_ptr<dx::RootSignature> _pAlphaTestLocalRootSignature;
	dx::WRL::ComPtr<ID3D12StateObject> _pRayTracingPSO;
    CallbackHandle                     _buildRenderSettingUiHandle;
    // clang-format on
};
