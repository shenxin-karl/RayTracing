#pragma once
#include <memory>
#include "Foundation/NonCopyable.h"
#include "D3d12/D3dStd.h"
#include "Foundation/Memory/SharedPtr.hpp"

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
	auto RequireBottomLevelAS(dx::IASBuilder *pIASBuilder) -> dx::BottomLevelAS *;
	auto GenerateBottomLevelAccelerationStructure(dx::IASBuilder *pIASBuilder) const -> SharedPtr<dx::BottomLevelAS>;
private:
	// clang-format off
	SharedPtr<dx::Buffer>				_pStaticBuffer;
	SharedPtr<dx::BottomLevelAS>		_pBottomLevelAS;
	D3D12_VERTEX_BUFFER_VIEW			_vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW				_indexBufferView;
	// clang-format on
};
