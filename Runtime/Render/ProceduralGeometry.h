#pragma once
#include "Renderer.h"


class ProceduralGeometry : public Renderer {
public:
	ProceduralGeometry();
	~ProceduralGeometry() override;
public:
	void OnCreate(uint32_t numBackBuffer, HWND hwnd) override;
	void OnDestroy() override;
	void OnPreRender(GameTimer &timer) override;
	void OnRender(GameTimer &timer) override;
	void OnResize(uint32_t width, uint32_t height) override;
private:

};
