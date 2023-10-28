#pragma once
#include "D3dUtils.h"
#include "Foundation/StringUtil.h"

namespace dx {

class StaticBuffer : NonCopyable {
public:
    StaticBuffer(std::source_location sl = std::source_location::current());
    ~StaticBuffer();
public:
    void OnCreate(Device *pDevice,
        size_t totalMemSize,
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
        UINT64 alignment = 0);
    void OnCreate(Device *pDevice, const D3D12_RESOURCE_DESC &desc);
    void OnDestroy();
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
private:
    // clang-format off
    std::string                   _name;
	Device						 *_pDevice;
    D3D12MA::Allocation *         _pAllocation;
    D3D12_RESOURCE_DESC           _bufferDesc;
    // clang-format on
};

class StaticBufferUploadHeap {
public:
    StaticBufferUploadHeap(UploadHeap &uploadHeap, StaticBuffer &staticBuffer, size_t staticBufferOffset = 0);
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

    void DoUpload();

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
    StaticBuffer   *_pStaticBuffer      = nullptr;
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
