#pragma once
#include <memory>
#include "Foundation/NonCopyable.h"
#include "D3d12/D3dStd.h"

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
	auto RequireBottomLevelAS(dx::IASBuilder *pIASBuilder, bool isOpaque) -> dx::BottomLevelAS *;
	auto GenerateBottomLevelAccelerationStructure(dx::IASBuilder *pIASBuilder, bool isOpaque) const -> std::shared_ptr<dx::BottomLevelAS>;
private:
	// clang-format off
	std::unique_ptr<dx::StaticBuffer>	_pStaticBuffer;
	std::shared_ptr<dx::BottomLevelAS>	_pOpaqueBottomLevelAS;
	std::shared_ptr<dx::BottomLevelAS>  _pTransparentBottomLevelAS;
	D3D12_VERTEX_BUFFER_VIEW			_vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW				_indexBufferView;
	// clang-format on
};
