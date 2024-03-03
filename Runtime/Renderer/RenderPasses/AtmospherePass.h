#pragma once
#include "RenderPass.h"
#include "D3d12/D3dStd.h"
#include "D3d12/Texture.h"
#include "Renderer/RenderUtils/RenderView.h"

struct AtmosphereParameter {
	// clang-format off
	float		seaLevel;                           // 海平面高度
    float		planetRadius;                       // 星球的半径
    float		atmosphereHeight;                   // 大气的高度
    float		sunLightIntensity;
    glm::vec3	sunLightColor;
    float		sunDiskAngle;
    float		rayleighScatteringScalarHeight;     // 瑞利散射标高,通常是 8500
    float		mieAnisotropy;                      // Mie 散射的各项异性 (0 ~ 1)
    float		mieScatteringScalarHeight;          // Mie 散射标高, 通常是 1200
    float		ozoneLevelCenterHeight;             // 臭氧的中心, 通常是 25km
    float		ozoneLevelWidth;                    // 臭氧的宽度, 通常是 15km
	// clang-format on
};

class AtmospherePass : public RenderPass {
public:
	AtmospherePass();
	void OnCreate();
	void OnDestroy() override;
public:
	struct AtmospherePassArgs {
		dx::ComputeContext *pComputeCtx = nullptr;
		RenderView *pRenderView = nullptr;
	};
	void GenerateLut(AtmospherePassArgs args);
private:
	void CreateSkyboxLutObject();
	void GenerateSkyBoxLut(const AtmospherePassArgs &args);
public:
	AtmosphereParameter parameter;
private:
	// clang-format off
	SharedPtr<dx::Texture>				 _pSkyBoxLutTex;
	SharedPtr<dx::RootSignature>		 _pSkyBoxLutRS;
	dx::WRL::ComPtr<ID3D12PipelineState> _pSkyBoxLutPSO;
	dx::SRV								 _skyboxLutSRV;
	dx::UAV								 _skyboxLutUAV;
	// clang-format on
};