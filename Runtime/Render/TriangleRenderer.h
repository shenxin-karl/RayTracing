#pragma once
#include "Renderer.h"

class TriangleRenderer : public Renderer {
private:
	void OnCreate(uint32_t numBackBuffer, HWND hwnd) override;
	void OnDestroy() override;
	void OnRender(GameTimer &timer) override;
private:
	std::shared_ptr<dx::StaticBuffer>		_pTriangleStaticBuffer;
	D3D12_VERTEX_BUFFER_VIEW				_vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW					_indexBufferView;
};

