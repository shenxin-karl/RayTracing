#pragma once
#include "Renderer.h"
#include "D3d12/Texture.h"
#include "D3d12/DescriptorHandle.h"
#include "RenderObject/ConstantBufferHelper.h"

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
	void InitScene();
	void SetupCamera();
	void SetupLight();
	void LoadGLTF();
private:
	// clang-format off
	dx::Texture			_renderTargetTex;
	dx::Texture			_depthStencilTex;
	dx::RTV				_renderTextureRTV;
	dx::DSV				_depthStencilDSV;

	cbuffer::CbPrePass	_cbPrePass;

	Scene			    *_pScene = nullptr;
	GameObject			*_pCameraGO = nullptr;
	// clang-format on
};
