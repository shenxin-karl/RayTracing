#pragma once
#include "Renderer.h"
#include "D3d12/DescriptorHandle.h"
#include "Foundation/Memory/SharedPtr.hpp"
#include "Renderer/RenderUtils/RenderView.h"

class ForwardPass;
class Scene;
class DeferredLightingPass;
class PostProcessPass;
class GBufferPass;
class AtmospherePass;

class SkyDemo : public Renderer {
public:
	SkyDemo();
	~SkyDemo() override;
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
	void LoadScene();
	void SetupCamera();
	void SetupLight();
	void RecreateWindowSizeDependentResources();
private:
	// clang-format off
	SharedPtr<dx::Texture>			_pRenderTargetTex;
	dx::RTV							_renderTargetRTV;
	dx::SRV							_renderTargetSRV;

	SharedPtr<dx::Texture>			_pDepthStencilTex;
	dx::DSV							_depthStencilDSV;
	dx::SRV							_depthStencilSRV;

	RenderView					     _renderView;
	ResolutionInfo					 _resolutionInfo;
	Scene							*_pScene;
	GameObject						*_pCameraGO;

	std::unique_ptr<ForwardPass>			_pForwardPass;
	std::unique_ptr<PostProcessPass>		_pPostProcessPass;
	std::unique_ptr<AtmospherePass>			_pAtmospherePass;
	// clang-format on
};
