#pragma once
#include "D3dUtils.h"

namespace dx {

class UploadHeap : public NonCopyable {
public:
    UploadHeap();
    ~UploadHeap();
public:
    struct BufferCopy {
        size_t size = 0;
        size_t offset = 0;
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
    void DoUpload();

    auto GetBasePtr() const -> uint8_t * {
        return _pDataCur;
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
    void AddPreUploadBarrier(const D3D12_RESOURCE_BARRIER &barrier) {
        _preUploadBarriers.push_back(barrier);
    }
    void AddPostUploadBarrier(const D3D12_RESOURCE_BARRIER &barrier) {
        _postUploadBarriers.push_back(barrier);
    }
private:
    Device *_pDevice = nullptr;
    uint8_t *_pDataCur = nullptr;
    uint8_t *_pDataEnd = nullptr;
    uint8_t *_pDataBegin = nullptr;
    D3D12MA::Allocation *_pBufferAllocation = nullptr;
    WRL::ComPtr<ID3D12GraphicsCommandList> _pCommandList = nullptr;
    WRL::ComPtr<ID3D12CommandAllocator> _pCommandAllocator = nullptr;
    std::vector<BufferCopy> _bufferCopies;
    std::vector<TextureCopy> _textureCopies;
    std::vector<D3D12_RESOURCE_BARRIER> _preUploadBarriers;
    std::vector<D3D12_RESOURCE_BARRIER> _postUploadBarriers;
};

}    // namespace dx