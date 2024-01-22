#pragma once
#include "D3d12/DescriptorHandle.h"
#include "Renderer/Renderer.h"
#include "D3d12/Texture.h"
#include "RenderObject/ConstantBufferHelper.h"

class Denoiser;
class RayTracingShadowPass;
class SkyBoxPass;
class DeferredLightingPass;
class PostProcessPass;
class GameObject;
class Scene;
class GBufferPass;
struct CameraState;

class SoftShadow : public Renderer {
public:
	SoftShadow();
	~SoftShadow() override;
	void OnCreate() override;
	void OnDestroy() override;
public:
	void OnUpdate(GameTimer &timer) override;
	void OnPreRender(GameTimer &timer) override;
	void OnRender(GameTimer &timer) override;
	void OnResize(uint32_t width, uint32_t height) override;
private:
	void PrepareFrame();
	void RenderFrame();

	void CreateRenderPass();
	void CreateScene();
	void SetupCamera();
	void SetupLight();
	void LoadGLTF();
	void LoadCubeMap();
	void RecreateWindowSizeDependentResources();
private:
	dx::Texture						_renderTargetTex;
	dx::RTV							_renderTargetRTV;
	dx::SRV							_renderTargetSRV;
	dx::UAV							_renderTargetUAV;

	dx::Texture						_depthStencilTex;
	dx::DSV							_depthStencilDSV;
	dx::SRV							_depthStencilSRV;

	cbuffer::CbPrePass				_cbPrePass;
	cbuffer::CbLighting				_cbLighting;
	std::unique_ptr<CameraState>	_pPreviousCameraState;
	std::unique_ptr<CameraState>	_pCurrentCameraState;

	Scene							*_pScene;
	GameObject						*_pCameraGO;

	std::shared_ptr<dx::Texture>	_pSkyBoxCubeMap;
	dx::SRV							_skyBoxCubeSRV;

	std::unique_ptr<GBufferPass>			_pGBufferPass;
	std::unique_ptr<PostProcessPass>		_pPostProcessPass;
	std::unique_ptr<DeferredLightingPass>	_pDeferredLightingPass;
	std::unique_ptr<SkyBoxPass>				_pSkyBoxPass;
	std::unique_ptr<RayTracingShadowPass>	_pRayTracingShadowPass;
	std::unique_ptr<Denoiser>				_pDenoiser;
};