#pragma once
#include "Renderer.h"

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
	Scene	*_pScene = nullptr;
	// clang-format on
};
