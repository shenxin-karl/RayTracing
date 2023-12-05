#pragma once
#include "Renderer.h"
#include "D3d12/AccelerationStructure.h"
#include "D3d12/DescriptorHandle.h"
#include "D3d12/RootSignature.h"
#include "D3d12/Texture.h"
#include "D3d12/ASBuilder.h"

class TriangleRenderer : public Renderer {
private:
	void OnCreate() override;
	void OnDestroy() override;
	void OnPreRender(GameTimer &timer) override;
	void OnRender(GameTimer &timer) override;
	void OnResize(uint32_t width, uint32_t height) override;
public:
	void CreateGeometry();
	void CreateRootSignature();
	void CreateRayTracingPipelineStateObject();
	void CreateRayTracingOutputResource();
	void BuildAccelerationStructures();

	struct Viewport {
	    float left;
	    float top;
	    float right;
	    float bottom;
	};

	struct RayGenConstantBuffer {
	    Viewport viewport;
	    Viewport stencil;
	};
private:
	// clang-format off
	std::shared_ptr<dx::StaticBuffer>		_pTriangleStaticBuffer;
	dx::RootSignature						_globalRootSignature;
	dx::RootSignature						_localRootSignature;
    dx::WRL::ComPtr<ID3D12StateObject>		_pRayTracingPSO;
	dx::Texture								_rayTracingOutput;
	dx::UAV									_rayTracingOutputView;
	RayGenConstantBuffer					_rayGenConstantBuffer = {};

	dx::BottomLevelAS						_bottomLevelAs;
	dx::TopLevelAS							_topLevelAs;

	D3D12_VERTEX_BUFFER_VIEW				_vertexBufferView = {};
	D3D12_INDEX_BUFFER_VIEW					_indexBufferView  = {};
	// clang-format on
};
