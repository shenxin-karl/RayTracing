#pragma once
#include "D3dStd.h"
#include "AccelerationStructure.h"

namespace dx {

class BottomLevelASGenerator : private NonCopyable {
public:
    void AddGeometry(D3D12_VERTEX_BUFFER_VIEW vbv, DXGI_FORMAT vertexFormat, bool isOpaque = true);
    void AddGeometry(D3D12_VERTEX_BUFFER_VIEW vbv, DXGI_FORMAT vertexFormat, D3D12_GPU_VIRTUAL_ADDRESS transformBuffer, bool isOpaque = true);
    void AddGeometry(D3D12_VERTEX_BUFFER_VIEW vbv, DXGI_FORMAT vertexFormat, D3D12_INDEX_BUFFER_VIEW ibv, bool isOpaque = true);
    void AddGeometry(D3D12_VERTEX_BUFFER_VIEW vbv,
        DXGI_FORMAT vertexFormat,
        D3D12_INDEX_BUFFER_VIEW ibv,
        D3D12_GPU_VIRTUAL_ADDRESS transformBuffer,
        bool isOpaque = true);

    auto CommitBuildCommand(IASBuilder *pASBuilder) -> std::shared_ptr<BottomLevelAS>;
private:
    void AddGeometryInternal(D3D12_VERTEX_BUFFER_VIEW *pVbv,
        DXGI_FORMAT vertexFormat, 
        D3D12_INDEX_BUFFER_VIEW *pIbv,
        D3D12_GPU_VIRTUAL_ADDRESS *pTransformBuffer,
        bool isOpaque);
private:
    // clang-format off
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>			_vertexBuffers		= {};
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS _flags				= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    // clang-format on
};

}    // namespace dx
