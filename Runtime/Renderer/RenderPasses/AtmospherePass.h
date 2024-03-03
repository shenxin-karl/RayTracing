#pragma once
#include "RenderPass.h"
#include "D3d12/D3dStd.h"
#include "D3d12/Texture.h"
#include "Renderer/RenderUtils/RenderView.h"

struct AtmosphereParameter {
	// clang-format off
	float		seaLevel;                           // ��ƽ��߶�
    float		planetRadius;                       // ����İ뾶
    float		atmosphereHeight;                   // �����ĸ߶�
    float		sunLightIntensity;
    glm::vec3	sunLightColor;
    float		sunDiskAngle;
    float		rayleighScatteringScalarHeight;     // ����ɢ����,ͨ���� 8500
    float		mieAnisotropy;                      // Mie ɢ��ĸ������� (0 ~ 1)
    float		mieScatteringScalarHeight;          // Mie ɢ����, ͨ���� 1200
    float		ozoneLevelCenterHeight;             // ����������, ͨ���� 25km
    float		ozoneLevelWidth;                    // �����Ŀ��, ͨ���� 15km
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