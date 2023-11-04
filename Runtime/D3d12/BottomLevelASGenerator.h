#pragma once
#include "D3dUtils.h"
#include "AccelerationStructure.h"

namespace dx {

class BottomLevelASGenerator : public NonCopyable {
public:
    void AddGeometry(D3D12_VERTEX_BUFFER_VIEW vbv, DXGI_FORMAT vertexFormat, bool isOpaque = true);
    void AddGeometry(D3D12_VERTEX_BUFFER_VIEW vbv, DXGI_FORMAT vertexFormat, D3D12_GPU_VIRTUAL_ADDRESS transformBuffer, bool isOpaque = true);
    void AddGeometry(D3D12_VERTEX_BUFFER_VIEW vbv, DXGI_FORMAT vertexFormat, D3D12_INDEX_BUFFER_VIEW ibv, bool isOpaque = true);
    void AddGeometry(D3D12_VERTEX_BUFFER_VIEW vbv,
        DXGI_FORMAT vertexFormat,
        D3D12_INDEX_BUFFER_VIEW ibv,
        D3D12_GPU_VIRTUAL_ADDRESS transformBuffer,
        bool isOpaque = true);

    void ComputeASBufferSizes(ASBuilder *pUploadHeap);
    auto Generate(ASBuilder *pUploadHeap) -> BottomLevelAS;

    auto GetScratchSizeInBytes() const -> size_t {
        return _scratchSizeInBytes;
    }
    auto GetResultSizeInBytes() const -> size_t {
        return _resultSizeInBytes;
    }
private:
    void AddGeometryInternal(D3D12_VERTEX_BUFFER_VIEW *pVbv,
        DXGI_FORMAT vertexFormat, 
        D3D12_INDEX_BUFFER_VIEW *pIbv,
        D3D12_GPU_VIRTUAL_ADDRESS *pTransformBuffer,
        bool isOpaque);
private:
    // clang-format off
	size_t												_scratchSizeInBytes	= 0;
	size_t												_resultSizeInBytes	= 0;
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>			_vertexBuffers		= {};
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS _flags				= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    // clang-format on
};

}    // namespace dx
