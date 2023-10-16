#pragma once
#include "Renderer.h"
#include "D3d12/RootSignature.h"

class TriangleRenderer : public Renderer {
private:
	void OnCreate(uint32_t numBackBuffer, HWND hwnd) override;
	void OnDestroy() override;
	void OnRender(GameTimer &timer) override;
public:
	void CreateGeometry();
	void CreateRootSignature();
	void CreateRayTracingPipelineStateObject();
private:
	// clang-format off
	std::shared_ptr<dx::StaticBuffer>		_pTriangleStaticBuffer;
	dx::RootSignature						_rootSignature;
    dx::WRL::ComPtr<ID3D12StateObject>		_pRayTracingPSO;

	D3D12_VERTEX_BUFFER_VIEW				_vertexBufferView = {};
	D3D12_INDEX_BUFFER_VIEW					_indexBufferView  = {};
	// clang-format on
};