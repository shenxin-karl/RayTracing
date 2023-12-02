#pragma once
#include "Renderer.h"

class Scene;
class HumanSkullPBR : public Renderer {
public:
	HumanSkullPBR();
	~HumanSkullPBR() override;
public:
	void OnCreate(uint32_t numBackBuffer, HWND hwnd) override;
	void OnDestroy() override;
	void OnPreRender(GameTimer &timer) override;
	void OnRender(GameTimer &timer) override;
	void OnResize(uint32_t width, uint32_t height) override;
private:
	void InitScene();
	void SetupCamera();
	void SetupLight();
private:
	// clang-format off
	Scene	*_pScene = nullptr;
	// clang-format on
};
