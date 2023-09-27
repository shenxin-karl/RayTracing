#pragma once
#include "D3dUtils.h"

namespace dx {

class StaticBuffer : public NonCopyable {
private:
    StaticBuffer();
    ~StaticBuffer();
public:
    void OnCreate(Device *pDevice, size_t totalMemSize, std::string_view name);
    void OnCreate(Device *pDevice, size_t totalMemSize, std::source_location sl = std::source_location::current());
    void OnDestroy();
    auto GetResource() const -> ID3D12Resource *;
private:
    // clang-format off
	Device						 *_pDevice;
	WRL::ComPtr<ID3D12Resource>   _pResource;
    D3D12MA::Allocation *         _pAllocation;
    D3D12_RESOURCE_DESC           _bufferDesc;
    // clang-format on
};

class StaticBufferUploadHeap {
public:
    StaticBufferUploadHeap(StaticBuffer &staticBuffer, UploadHeap &uploadHeap);
    ~StaticBufferUploadHeap();
public:
    auto AllocVertexBuffer(size_t numOfVertices, size_t strideInBytes, const void *pData) -> std::optional<D3D12_VERTEX_BUFFER_VIEW>;
    auto AllocIndexBuffer(size_t numOfVertices, size_t strideInBytes, const void *pData) -> std::optional<D3D12_INDEX_BUFFER_VIEW>;
    auto AllocConstantBuffer(size_t totalMemory, const void *pData) -> std::optional<D3D12_CONSTANT_BUFFER_VIEW_DESC>;
    auto AllocStructuredBuffer(size_t numOfVertices, size_t strideInBytes, const void *pData) -> std::optional<SRV>;
    void Finalize();

    template<typename T>
    auto AllocVertexBuffer(std::span<T> view) -> std::optional<D3D12_VERTEX_BUFFER_VIEW> {
	    return AllocVertexBuffer(view.size(), sizeof(T), view.data());
    }
    template<std::integral T>
    auto AllocIndexBuffer(std::span<T> view) -> std::optional<D3D12_INDEX_BUFFER_VIEW> {
	    return AllocIndexBuffer(view.size(), sizeof(T), view.data());
    }
    template<typename T>
    auto AllocConstantBuffer(const T &object) -> std::optional<D3D12_CONSTANT_BUFFER_VIEW_DESC> {
	    return AllocConstantBuffer(sizeof(T), &object);
    }
    template<typename T>
    auto AllocStructuredBuffer(std::span<T> view) -> std::optional<SRV> {
	    return AllocStructuredBuffer(view.size(), sizeof(T), view.data());
    }
private:
    // clang-format off
    size_t  _offset             = 0;
    bool    _uploadFinished     = false;
    bool    _vertexBuffer       = false;
    bool    _indexBuffer        = false;
    bool    _constantBuffer     = false;
    bool    _structuredBuffer   = false;
    // clang-format on
};

}    // namespace dx
