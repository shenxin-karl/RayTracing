#pragma once
#include "Renderer.h"
#include "D3d12/Texture.h"
#include "D3d12/DescriptorHandle.h"
#include "RenderObject/ConstantBufferHelper.h"

class PostProcessPass;
class ForwardPass;
class Scene;
class GLTFSample : public Renderer {
public:
	GLTFSample();
	~GLTFSample() override;
public:
	void OnCreate() override;
	void OnDestroy() override;
	void OnPreRender(GameTimer &timer) override;
	void OnRender(GameTimer &timer) override;
	void OnResize(uint32_t width, uint32_t height) override;
private:
	void InitRenderPass();
	void InitScene();
	void SetupCamera();
	void SetupLight();
	void LoadGLTF();
private:
	// clang-format off
	dx::Texture					 _renderTargetTex;
	dx::Texture					 _depthStencilTex;
	dx::RTV						 _renderTextureRTV;
	dx::SRV						 _renderTextureSRV;
	dx::DSV						 _depthStencilDSV;

	cbuffer::CbPrePass			 _cbPrePass;
	cbuffer::CbLighting			 _cbLighting;


	Scene						*_pScene = nullptr;
	GameObject					*_pCameraGO = nullptr;

	// render passes
	std::unique_ptr<ForwardPass>	 _pForwardPass;
	std::unique_ptr<PostProcessPass> _pPostProcessPass;
	// clang-format on
};
