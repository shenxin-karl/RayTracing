#pragma once
#include "D3dUtils.h"
#include "Fence.h"

namespace dx {

// todo: 需要修改为支持异步构建
class UploadHeap : NonCopyable {
public:
    UploadHeap();
    ~UploadHeap();
public:
    struct BufferCopy {
        size_t size = 0;
        size_t srcOffset = 0;
        size_t dstOffset = 0;
        ID3D12Resource *pDestBuffer = nullptr;
    };
    struct TextureCopy {
        CD3DX12_TEXTURE_COPY_LOCATION src;
        CD3DX12_TEXTURE_COPY_LOCATION dst;
    };
public:
    void OnCreate(Device *pDevice, size_t size);
    void OnDestroy();
    auto AllocBuffer(size_t size, size_t align = 1) -> uint8_t *;
    auto GetAllocatableSize(size_t align = 1) const -> size_t;
    void FlushAndFinish();

    auto GetDevice() const -> Device * {
	    return _pDevice;
    }
    auto GetCopyCommandList() const -> NativeCommandList * {
        return _pCommandList.Get();
    }
    auto GetCommandAllocator() const -> ID3D12CommandAllocator * {
	    return _pCommandAllocator.Get();
    }
    auto CalcBufferOffset(const uint8_t *ptr) const -> size_t {
        return ptr - _pDataBegin;
    }
    auto GetBasePtr() const -> uint8_t * {
        return _pDataBegin;
    }
    auto GetResource() const -> ID3D12Resource * {
        return _pBufferAllocation->GetResource();
    }
    void AddBufferCopy(const BufferCopy &bufferCopy) {
        _bufferCopies.push_back(bufferCopy);
    }
    void AddTextureCopy(const TextureCopy &textureCopy) {
        _textureCopies.push_back(textureCopy);
    }
    void AddPreUploadTranslation(ID3D12Resource *pResource,
        D3D12_RESOURCE_STATES stateAfter,
        UINT subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
        D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE);

    void AddPostUploadTranslation(ID3D12Resource *pResource,
        D3D12_RESOURCE_STATES stateAfter,
        UINT subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
        D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE);
private:
    // clang-format off
    Device                                  *_pDevice               = nullptr;
    uint8_t                                 *_pDataCur              = nullptr;
    uint8_t                                 *_pDataEnd              = nullptr;
    uint8_t                                 *_pDataBegin            = nullptr;
    D3D12MA::Allocation                     *_pBufferAllocation     = nullptr;
    WRL::ComPtr<NativeCommandList>           _pCommandList          = nullptr;
    WRL::ComPtr<ID3D12CommandAllocator>      _pCommandAllocator     = nullptr;
    std::unique_ptr<Fence>                   _pUploadFinishedFence  = nullptr;
    std::vector<BufferCopy>                  _bufferCopies;
    std::vector<TextureCopy>                 _textureCopies;
    std::vector<D3D12_RESOURCE_BARRIER>      _preUploadBarriers;
    std::vector<D3D12_RESOURCE_BARRIER>      _postUploadBarriers;
    // clang-format on
};

}    // namespace dx