#pragma once
#include <memory>
#include "Foundation/ITick.hpp"
#include "D3d12/D3dStd.h"

class Renderer : public ITick {
public:
	Renderer();
	~Renderer() override;
public:
	virtual void OnCreate();
	virtual void OnDestroy();
	virtual void OnPreRender(GameTimer &timer) override;
	virtual void OnPostRender(GameTimer &timer) override;
	virtual void OnResize(uint32_t width, uint32_t height);
	static void ShowFPS();
protected:
	// clang-format off
	size_t									_width;
	size_t									_height;
	dx::Device							   *_pDevice;
	dx::SwapChain						   *_pSwapChain;
	dx::UploadHeap						   *_pUploadHeap;
	dx::SyncASBuilder					   *_pASBuilder;
	std::unique_ptr<dx::FrameResourceRing>	_pFrameResourceRing;
	// clang-format on
};
