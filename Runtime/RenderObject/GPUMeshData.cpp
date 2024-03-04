#include "GPUMeshData.h"
#include "CPUMeshData.h"
#include "D3d12/ASBuilder.h"
#include "D3d12/BottomLevelASGenerator.h"
#include "D3d12/Buffer.h"
#include "Renderer/GfxDevice.h"
#include "Foundation/Formatter.hpp"

GPUMeshData::GPUMeshData() : _vertexBufferView{}, _indexBufferView{} {
}

GPUMeshData::~GPUMeshData() {
}

void GPUMeshData::SetName(std::string_view name) {
    if (_pBottomLevelAS != nullptr) {
        std::string opaqueBottomLevelASName = fmt::format("{}_BottomLevelAS", name);
        _pBottomLevelAS->SetName(opaqueBottomLevelASName);
    }
    if (_pStaticBuffer != nullptr) {
        _pStaticBuffer->SetName(name);
    }
}

auto GPUMeshData::GetVertexBufferView() const -> D3D12_VERTEX_BUFFER_VIEW {
    return _vertexBufferView;
}

auto GPUMeshData::GetIndexBufferView() const -> D3D12_INDEX_BUFFER_VIEW {
    return _indexBufferView;
}

auto GPUMeshData::GetBottomLevelAS() const -> dx::BottomLevelAS * {
    return _pBottomLevelAS.Get();
}

void GPUMeshData::UploadGpuMemory(const CPUMeshData *pMeshData) {
    SemanticMask semanticMask = pMeshData->GetSemanticMask();
    size_t vertexStride = GetSemanticStride(semanticMask);
    size_t vertexCount = pMeshData->GetVertexCount();
    size_t indexCount = pMeshData->GetIndexCount();
    size_t indexStride = sizeof(uint32_t);
    size_t vertexBufferSize = vertexCount * vertexStride;
    size_t indexBufferSize = indexCount * indexStride;

    GfxDevice *pGfxDevice = GfxDevice::GetInstance();
    _pStaticBuffer = dx::Buffer::CreateStatic(pGfxDevice->GetDevice(), vertexBufferSize + indexBufferSize);

    dx::StaticBufferUploadHeap uploadHeap(pGfxDevice->GetUploadHeap(), _pStaticBuffer.Get());
    _vertexBufferView = uploadHeap.AllocVertexBuffer(vertexCount, vertexStride, pMeshData->GetVertices()).value();

    _indexBufferView = {};
    if (indexCount > 0) {
        _indexBufferView = uploadHeap.AllocIndexBuffer(indexCount, indexStride, pMeshData->GetIndices()).value();
    }
    uploadHeap.CommitUploadCommand();
}

auto GPUMeshData::RequireBottomLevelAS(dx::IASBuilder *pIASBuilder) -> dx::BottomLevelAS * {
    std::string_view name = _pStaticBuffer->GetName();
    if (_pBottomLevelAS == nullptr) {
        std::string opaqueBottomLevelASName = fmt::format("{}_BottomLevelAS", name);
        _pBottomLevelAS = GenerateBottomLevelAccelerationStructure(pIASBuilder);
        _pBottomLevelAS->SetName(opaqueBottomLevelASName);
    }
    return _pBottomLevelAS.Get();
}

auto GPUMeshData::GenerateBottomLevelAccelerationStructure(dx::IASBuilder *pIASBuilder) const
    -> SharedPtr<dx::BottomLevelAS> {
    constexpr DXGI_FORMAT vertexFormat = GetSemanticInfo(SemanticIndex::eVertex).format;
    dx::BottomLevelASGenerator generator;
    if (_indexBufferView.SizeInBytes > 0) {
        generator.AddGeometry(_vertexBufferView, vertexFormat, _indexBufferView);
    } else {
        generator.AddGeometry(_vertexBufferView, vertexFormat);
    }
    return generator.CommitBuildCommand(pIASBuilder);
}
