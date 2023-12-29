#include "BottomLevelASGenerator.h"
#include "AccelerationStructure.h"
#include "ASBuilder.h"
#include "Device.h"

namespace dx {

void BottomLevelASGenerator::AddGeometry(D3D12_VERTEX_BUFFER_VIEW vbv, DXGI_FORMAT vertexFormat, bool isOpaque) {
    return AddGeometryInternal(&vbv, vertexFormat, nullptr, nullptr, isOpaque);
}

void BottomLevelASGenerator::AddGeometry(D3D12_VERTEX_BUFFER_VIEW vbv,
    DXGI_FORMAT vertexFormat,
    D3D12_GPU_VIRTUAL_ADDRESS transformBuffer,
    bool isOpaque) {
    return AddGeometryInternal(&vbv, vertexFormat, nullptr, &transformBuffer, isOpaque);
}

void BottomLevelASGenerator::AddGeometry(D3D12_VERTEX_BUFFER_VIEW vbv,
    DXGI_FORMAT vertexFormat,
    D3D12_INDEX_BUFFER_VIEW ibv,
    bool isOpaque) {
    return AddGeometryInternal(&vbv, vertexFormat, &ibv, nullptr, isOpaque);
}

void BottomLevelASGenerator::AddGeometry(D3D12_VERTEX_BUFFER_VIEW vbv,
    DXGI_FORMAT vertexFormat,
    D3D12_INDEX_BUFFER_VIEW ibv,
    D3D12_GPU_VIRTUAL_ADDRESS transformBuffer,
    bool isOpaque) {
    return AddGeometryInternal(&vbv, vertexFormat, &ibv, &transformBuffer, isOpaque);
}

auto BottomLevelASGenerator::CommitCommand(ASBuilder *pUploadHeap) -> BottomLevelAS {
#if ENABLE_RAY_TRACING
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS preBuildDesc;
    preBuildDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    preBuildDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    preBuildDesc.NumDescs = static_cast<UINT>(_vertexBuffers.size());
    preBuildDesc.pGeometryDescs = _vertexBuffers.data();
    preBuildDesc.Flags = _flags;

    NativeDevice *device = pUploadHeap->GetDevice()->GetNativeDevice();
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
    device->GetRaytracingAccelerationStructurePrebuildInfo(&preBuildDesc, &info);
    Assert(info.ResultDataMaxSizeInBytes > 0);
    info.ResultDataMaxSizeInBytes = AlignUp(info.ResultDataMaxSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    BottomLevelAS result;
    result.OnCreate(pUploadHeap->GetDevice(), info.ResultDataMaxSizeInBytes);

    ASBuilder::BottomASBuildItem buildItem;
    buildItem.scratchBufferSize = info.ScratchDataSizeInBytes;
    buildItem.pResource = result.GetResource();
    buildItem.vertexBuffers = std::move(_vertexBuffers);
    buildItem.flags = _flags;
    pUploadHeap->AddBuildItem(std::move(buildItem));
    return result;
#endif
}

void BottomLevelASGenerator::AddGeometryInternal(D3D12_VERTEX_BUFFER_VIEW *pVbv,
    DXGI_FORMAT vertexFormat,
    D3D12_INDEX_BUFFER_VIEW *pIbv,
    D3D12_GPU_VIRTUAL_ADDRESS *pTransformBuffer,
    bool isOpaque) {

    D3D12_RAYTRACING_GEOMETRY_DESC desc = {};
    desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    if (pIbv != nullptr) {
        desc.Triangles.IndexBuffer = pIbv->BufferLocation;
        desc.Triangles.IndexFormat = pIbv->Format;
        switch (pIbv->Format) {
        case DXGI_FORMAT_R8_UINT:
            desc.Triangles.IndexCount = pIbv->SizeInBytes / sizeof(std::uint8_t);
            break;
        case DXGI_FORMAT_R16_SINT:
        case DXGI_FORMAT_R16_UINT:
            desc.Triangles.IndexCount = pIbv->SizeInBytes / sizeof(std::uint16_t);
            break;
        default:
            Exception::Throw("Invalid Index Buffer Format!");
        }
    }
    desc.Triangles.Transform3x4 = (pTransformBuffer != nullptr) ? *pTransformBuffer : 0;
    desc.Triangles.VertexFormat = vertexFormat;
    desc.Triangles.VertexCount = pVbv->SizeInBytes / pVbv->StrideInBytes;
    desc.Triangles.VertexBuffer.StrideInBytes = pVbv->StrideInBytes;
    desc.Triangles.VertexBuffer.StartAddress = pVbv->BufferLocation;
    desc.Flags = isOpaque ? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE : D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
    _vertexBuffers.push_back(desc);
}

}    // namespace dx
