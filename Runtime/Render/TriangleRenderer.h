#pragma once
#include "Renderer.h"
#include "D3d12/DescriptorHandle.h"
#include "D3d12/RootSignature.h"
#include "D3d12/Texture.h"

class TriangleRenderer : public Renderer {
private:
	void OnCreate(uint32_t numBackBuffer, HWND hwnd) override;
	void OnDestroy() override;
	void OnRender(GameTimer &timer) override;
public:
	void CreateGeometry();
	void CreateRootSignature();
	void CreateRayTracingPipelineStateObject();
	void CreateRayTracingOutputResource();
	void BuildAccelerationStructures();
private:
	// clang-format off
	std::shared_ptr<dx::StaticBuffer>		_pTriangleStaticBuffer;
	dx::RootSignature						_globalRootSignature;
	dx::RootSignature						_localRootSignature;
    dx::WRL::ComPtr<ID3D12StateObject>		_pRayTracingPSO;
	dx::Texture								_rayTracingOutput;
	dx::UAV									_rayTracingOutputView;
	dx::WRL::ComPtr<D3D12MA::Allocation>	_pTopLevelAccelerationStructure;
	dx::WRL::ComPtr<D3D12MA::Allocation>	_pBottomLevelAccelerationStructure;

	D3D12_VERTEX_BUFFER_VIEW				_vertexBufferView = {};
	D3D12_INDEX_BUFFER_VIEW					_indexBufferView  = {};
	// clang-format on
};
