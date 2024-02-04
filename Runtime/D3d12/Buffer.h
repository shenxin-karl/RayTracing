#pragma once
#include "D3dStd.h"
#include "Foundation/StringUtil.h"
#include "Foundation/Memory/SharedPtr.hpp"

namespace dx {

class Buffer : public RefCounter {
    Buffer(Device *pDevice, D3D12_HEAP_TYPE heapType, const D3D12_RESOURCE_DESC &desc);
    Buffer(Device *pDevice, D3D12_HEAP_TYPE heapType, size_t bufferSize)
        : Buffer(pDevice, heapType, CD3DX12_RESOURCE_DESC::Buffer(bufferSize)) {
    }
public:
    template<typename... Args>
    static SharedPtr<Buffer> CreateStatic(Device *pDevice, Args &&...args) {
        return SharedPtr<Buffer>(new Buffer(pDevice, D3D12_HEAP_TYPE_DEFAULT, std::forward<Args>(args)...));
    }
    template<typename... Args>
    static SharedPtr<Buffer> CreateDynamic(Device *pDevice, Args &&...args) {
        return SharedPtr<Buffer>(new Buffer(pDevice, D3D12_HEAP_TYPE_UPLOAD, std::forward<Args>(args)...));
    }
    template<typename... Args>
    static SharedPtr<Buffer> CreateReadback(Device *pDevice, Args &&...args) {
        return SharedPtr<Buffer>(new Buffer(pDevice, D3D12_HEAP_TYPE_READBACK, std::forward<Args>(args)...));
    }
    ~Buffer() override;
public:
    Inline(2) auto GetResource() const -> ID3D12Resource * {
        return _pAllocation != nullptr ? _pAllocation->GetResource() : nullptr;
    }
    Inline(2) void SetName(std::string_view name) {
        _name = name;
        _pAllocation->GetResource()->SetName(nstd::to_wstring(name).c_str());
    }
    Inline(2) auto GetDesc() const -> const D3D12_RESOURCE_DESC & {
        return _bufferDesc;
    }
    Inline(2) auto GetDevice() const -> Device * {
        return _pDevice;
    }
    Inline(2) auto GetName() const -> std::string_view {
        return _name;
    }
    Inline(2) bool IsStaticBuffer() const {
        return _heapType == D3D12_HEAP_TYPE_DEFAULT;
    }
    Inline(2) bool IsDynamicBuffer() const {
        return _heapType == D3D12_HEAP_TYPE_UPLOAD;
    }
    Inline(2) bool IsReadbackBuffer() const {
        return _heapType == D3D12_HEAP_TYPE_READBACK;
    }
    Inline(2) auto GetBufferSize() const -> size_t {
	    return _bufferDesc.Width;
    }
private:
    // clang-format off
    std::string                   _name;
	Device						 *_pDevice;
    D3D12_HEAP_TYPE               _heapType;
    D3D12MA::Allocation *         _pAllocation;
    D3D12_RESOURCE_DESC           _bufferDesc;
    // clang-format on
};

class StaticBufferUploadHeap {
public:
    StaticBufferUploadHeap(UploadHeap *pUploadHeap, Buffer *pStaticBuffer, size_t staticBufferOffset = 0);
    ~StaticBufferUploadHeap();
public:
    [[nodiscard]]
    auto AllocVertexBuffer(size_t numOfVertices, size_t strideInBytes, const void *pData)
        -> std::optional<D3D12_VERTEX_BUFFER_VIEW>;

    [[nodiscard]]
    auto AllocIndexBuffer(size_t numOfVertices, size_t strideInBytes, const void *pData)
        -> std::optional<D3D12_INDEX_BUFFER_VIEW>;

    [[nodiscard]]
    auto AllocConstantBuffer(size_t totalMemory, const void *pData) -> std::optional<D3D12_CONSTANT_BUFFER_VIEW_DESC>;
    [[nodiscard]]
    auto AllocStructuredBuffer(size_t numOfVertices, size_t strideInBytes, const void *pData) -> std::optional<SRV>;

    void CommitUploadCommand();

    template<typename T>
    [[nodiscard]]
    auto AllocVertexBuffer(std::span<T> view) -> std::optional<D3D12_VERTEX_BUFFER_VIEW> {
        return AllocVertexBuffer(view.size(), sizeof(T), view.data());
    }
    template<std::integral T>
    [[nodiscard]]
    auto AllocIndexBuffer(std::span<T> view) -> std::optional<D3D12_INDEX_BUFFER_VIEW> {
        return AllocIndexBuffer(view.size(), sizeof(T), view.data());
    }
    template<typename T>
    [[nodiscard]]
    auto AllocConstantBuffer(const T &object) -> std::optional<D3D12_CONSTANT_BUFFER_VIEW_DESC> {
        return AllocConstantBuffer(sizeof(T), &object);
    }
    template<typename T>
    [[nodiscard]]
    auto AllocStructuredBuffer(std::span<T> view) -> std::optional<SRV> {
        return AllocStructuredBuffer(view.size(), sizeof(T), view.data());
    }
private:
    struct BufferInResourceInfo {
        size_t bufferSize;
        D3D12_GPU_VIRTUAL_ADDRESS virtualAddress;
    };
    auto CopyToUploadBuffer(size_t numOfVertices, size_t strideInBytes, const void *pData, size_t dataSize)
        -> std::optional<BufferInResourceInfo>;
    auto CopyToUploadBuffer(size_t numOfVertices, size_t strideInBytes, const void *pData)
        -> std::optional<BufferInResourceInfo>;
private:
    // clang-format off
    Buffer         *_pStaticBuffer      = nullptr;
    UploadHeap     *_pUploadHeap        = nullptr;
    size_t          _srcOffset          = 0;
    size_t          _dstOffset          = 0;
    bool            _uploadFinished     = false;
    bool            _vertexBuffer       = false;
    bool            _indexBuffer        = false;
    bool            _constantBuffer     = false;
    bool            _structuredBuffer   = false;
    // clang-format on
};

}    // namespace dx
