#pragma once
#include <memory>
#include "Foundation/ITick.hpp"
#include "D3d12/D3dUtils.h"

class Renderer : public ITick {
public:
	Renderer();
	~Renderer() override;
public:
	void OnCreate(uint32_t numBackBuffer, HWND hwnd);
	void OnDestroy();
	void OnPreUpdate(GameTimer &timer) override;
	void OnUpdate(GameTimer &timer) override;
	void OnPostUpdate(GameTimer &timer) override;
	void OnPreRender(GameTimer &timer) override;
	void OnRender(GameTimer &timer) override;
	void OnPostRender(GameTimer &timer) override;
	void OnResize(uint32_t width, uint32_t height);
private:
	// clang-format off
	size_t									_width;
	size_t									_height;
	size_t									_numBackBuffer;
	std::unique_ptr<dx::Device>				_pDevice;
	std::unique_ptr<dx::SwapChain>			_pSwapChain;
	std::unique_ptr<dx::FrameResourceRing>	_pFrameResourceRing;
	std::unique_ptr<dx::UploadHeap>			_pUploadHeap;

private:
	void InitTriangleGeometry();
private:
	std::shared_ptr<dx::StaticBuffer>		_pTriangleStaticBuffer;
	D3D12_VERTEX_BUFFER_VIEW				_vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW					_indexBufferView;
	// clang-format on
};
