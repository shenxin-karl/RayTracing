#pragma once
#include <memory>
#include <d3d12.h>
#include <string_view>

#include "Foundation/NonCopyable.h"

namespace dx {
class StaticBuffer;
class BottomLevelAS;
}

class CPUMeshData;
class GPUMeshData : private NonCopyable {
public:
	GPUMeshData();
	~GPUMeshData();
public:
	void SetName(std::string_view name);
	auto GetVertexBufferView() const -> D3D12_VERTEX_BUFFER_VIEW;
	auto GetIndexBufferView() const -> D3D12_INDEX_BUFFER_VIEW;
private:
	friend class Mesh;
	void UploadGpuMemory(const CPUMeshData *pMeshData);
	void GenerateBottomLevelAccelerationStructure(bool isOpaque = true);
private:
	// clang-format off
	std::unique_ptr<dx::StaticBuffer>	_pStaticBuffer;
	std::unique_ptr<dx::BottomLevelAS>	_pBottomLevelAS;
	D3D12_VERTEX_BUFFER_VIEW			_vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW				_indexBufferView;
	// clang-format on
};
