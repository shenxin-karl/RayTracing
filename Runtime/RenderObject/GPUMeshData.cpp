#include "GPUMeshData.h"
#include "CPUMeshData.h"
#include "D3d12/ASBuilder.h"
#include "D3d12/BottomLevelASGenerator.h"
#include "D3d12/StaticBuffer.h"
#include "Renderer/GfxDevice.h"

GPUMeshData::GPUMeshData() : _vertexBufferView{}, _indexBufferView{} {
}

GPUMeshData::~GPUMeshData() {
}

void GPUMeshData::UploadGpuMemory(const CPUMeshData *pMeshData) {
    SemanticMask semanticMask = pMeshData->GetSemanticMask();
    size_t vertexStride = GetSemanticStride(semanticMask);
    size_t vertexCount = pMeshData->GetVertexCount();
    size_t indexCount = pMeshData->GetIndexCount();
    size_t indexStride = sizeof(int16_t);
    size_t vertexBufferSize = vertexCount * vertexStride;
    size_t indexBufferSize = indexCount * indexStride;

    GfxDevice *pGfxDevice = GfxDevice::GetInstance();
    _pStaticBuffer = std::make_unique<dx::StaticBuffer>();
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize + indexBufferSize);
    _pStaticBuffer->OnCreate(pGfxDevice->GetDevice(), bufferDesc);

    dx::StaticBufferUploadHeap uploadHeap(pGfxDevice->GetUploadHeap(), _pStaticBuffer.get());
    _vertexBufferView = uploadHeap.AllocVertexBuffer(vertexCount, vertexStride, pMeshData->GetVertices()).value();

    _indexBufferView = {};
    if (indexCount > 0) {
        _indexBufferView = uploadHeap.AllocIndexBuffer(indexCount, indexStride, pMeshData->GetIndices()).value();
    }
    uploadHeap.CommitUploadCommand();
}

void GPUMeshData::GenerateBottomLevelAccelerationStructure(bool isOpaque) {
    GfxDevice *pGfxDevice = GfxDevice::GetInstance();
    dx::ASBuilder *pASBuilder = pGfxDevice->GetASBuilder();
    constexpr DXGI_FORMAT vertexFormat = GetSemanticInfo(SemanticIndex::eVertex).format;
    dx::BottomLevelASGenerator generator;
    if (_indexBufferView.SizeInBytes > 0) {
        generator.AddGeometry(_vertexBufferView, vertexFormat, _indexBufferView, isOpaque);
    } else {
        generator.AddGeometry(_vertexBufferView, vertexFormat, isOpaque);
    }
    generator.CommitCommand(pASBuilder);
}
