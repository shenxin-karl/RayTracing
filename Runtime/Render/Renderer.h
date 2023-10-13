#pragma once
#include <memory>
#include "Foundation/ITick.hpp"
#include "D3d12/D3dUtils.h"

class Renderer : public ITick {
public:
	Renderer();
	~Renderer() override;
public:
	virtual void OnCreate(uint32_t numBackBuffer, HWND hwnd);
	virtual void OnDestroy();
	virtual void OnRender(GameTimer &timer) override;
	virtual void OnPostRender(GameTimer &timer) override;
	virtual void OnResize(uint32_t width, uint32_t height);
protected:
	// clang-format off
	size_t									_width;
	size_t									_height;
	size_t									_numBackBuffer;
	std::unique_ptr<dx::Device>				_pDevice;
	std::unique_ptr<dx::SwapChain>			_pSwapChain;
	std::unique_ptr<dx::FrameResourceRing>	_pFrameResourceRing;
	std::unique_ptr<dx::UploadHeap>			_pUploadHeap;
	// clang-format on
};
