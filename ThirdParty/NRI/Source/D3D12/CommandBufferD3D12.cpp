﻿// © 2021 NVIDIA Corporation

#include "SharedD3D12.h"
#include "CommandBufferD3D12.h"
#include "BufferD3D12.h"
#include "TextureD3D12.h"
#include "DescriptorD3D12.h"
#include "DescriptorSetD3D12.h"
#include "DescriptorPoolD3D12.h"
#include "PipelineD3D12.h"
#include "PipelineLayoutD3D12.h"
#include "QueryPoolD3D12.h"
#include "AccelerationStructureD3D12.h"

#include <pix.h>

using namespace nri;

Result CommandBufferD3D12::Create(D3D12_COMMAND_LIST_TYPE commandListType, ID3D12CommandAllocator* commandAllocator)
{
    ComPtr<ID3D12GraphicsCommandList> graphicsCommandList;
    HRESULT hr = ((ID3D12Device*)m_Device)->CreateCommandList(NRI_TEMP_NODE_MASK, commandListType, commandAllocator, nullptr, IID_PPV_ARGS(&graphicsCommandList));
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreateCommandList()");

    m_CommandAllocator = commandAllocator;
    m_GraphicsCommandList = graphicsCommandList;
    m_GraphicsCommandList->QueryInterface(IID_PPV_ARGS(&m_GraphicsCommandList1));
    m_GraphicsCommandList->QueryInterface(IID_PPV_ARGS(&m_GraphicsCommandList4));
    m_GraphicsCommandList->QueryInterface(IID_PPV_ARGS(&m_GraphicsCommandList6));

    hr = m_GraphicsCommandList->Close();
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12GraphicsCommandList::Close()");

    return Result::SUCCESS;
}

Result CommandBufferD3D12::Create(const CommandBufferD3D12Desc& commandBufferDesc)
{
    m_CommandAllocator = commandBufferDesc.d3d12CommandAllocator;
    m_GraphicsCommandList = commandBufferDesc.d3d12CommandList;
    m_GraphicsCommandList->QueryInterface(IID_PPV_ARGS(&m_GraphicsCommandList1));
    m_GraphicsCommandList->QueryInterface(IID_PPV_ARGS(&m_GraphicsCommandList4));
    m_GraphicsCommandList->QueryInterface(IID_PPV_ARGS(&m_GraphicsCommandList6));

    return Result::SUCCESS;
}

inline void CommandBufferD3D12::AddResourceBarrier(ID3D12Resource* resource, AccessBits before, AccessBits after, D3D12_RESOURCE_BARRIER& resourceBarrier, uint32_t subresource)
{
    D3D12_COMMAND_LIST_TYPE commandListType = m_GraphicsCommandList->GetType();
    D3D12_RESOURCE_STATES resourceStateBefore = GetResourceStates(before, commandListType);
    D3D12_RESOURCE_STATES resourceStateAfter = GetResourceStates(after, commandListType);

    if (resourceStateBefore == resourceStateAfter && resourceStateBefore == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
    {
        resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        resourceBarrier.UAV.pResource = resource;
    }
    else
    {
        resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        resourceBarrier.Transition.pResource = resource;
        resourceBarrier.Transition.StateBefore = resourceStateBefore;
        resourceBarrier.Transition.StateAfter = resourceStateAfter;
        resourceBarrier.Transition.Subresource = subresource;
    }
}

//================================================================================================================
// NRI
//================================================================================================================

inline Result CommandBufferD3D12::Begin(const DescriptorPool* descriptorPool)
{
    HRESULT hr = m_GraphicsCommandList->Reset(m_CommandAllocator, nullptr);
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12GraphicsCommandList::Reset()");

    if (descriptorPool)
        SetDescriptorPool(*descriptorPool);

    m_PipelineLayout = nullptr;
    m_IsGraphicsPipelineLayout = false;
    m_Pipeline = nullptr;
    m_PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

    return Result::SUCCESS;
}

inline Result CommandBufferD3D12::End()
{
    if (FAILED(m_GraphicsCommandList->Close()))
        return Result::FAILURE;

    return Result::SUCCESS;
}

inline void CommandBufferD3D12::SetViewports(const Viewport* viewports, uint32_t viewportNum)
{
    static_assert(offsetof(Viewport, x) == 0, "Unsupported viewport data layout.");
    static_assert(offsetof(Viewport, width) == 8, "Unsupported viewport data layout.");
    static_assert(offsetof(Viewport, depthRangeMin) == 16, "Unsupported viewport data layout.");
    static_assert(offsetof(Viewport, depthRangeMax) == 20, "Unsupported viewport data layout.");

    m_GraphicsCommandList->RSSetViewports(viewportNum, (D3D12_VIEWPORT*)viewports);
}

inline void CommandBufferD3D12::SetScissors(const Rect* rects, uint32_t rectNum)
{
    D3D12_RECT* rectsD3D12 = STACK_ALLOC(D3D12_RECT, rectNum);
    ConvertRects(rectsD3D12, rects, rectNum);

    m_GraphicsCommandList->RSSetScissorRects(rectNum, rectsD3D12);
}

inline void CommandBufferD3D12::SetDepthBounds(float boundsMin, float boundsMax)
{
    if (m_GraphicsCommandList1)
        m_GraphicsCommandList1->OMSetDepthBounds(boundsMin, boundsMax);
}

inline void CommandBufferD3D12::SetStencilReference(uint8_t reference)
{
    m_GraphicsCommandList->OMSetStencilRef(reference);
}

inline void CommandBufferD3D12::SetSamplePositions(const SamplePosition* positions, uint32_t positionNum)
{
    if (m_GraphicsCommandList1)
    {
        Sample_t sampleNum = m_Pipeline->GetSampleNum();
        uint32_t pixelNum = positionNum / sampleNum;

        static_assert(sizeof(D3D12_SAMPLE_POSITION) == sizeof(SamplePosition));
        m_GraphicsCommandList1->SetSamplePositions(sampleNum, pixelNum, (D3D12_SAMPLE_POSITION*)positions);
    }
}

inline void CommandBufferD3D12::ClearAttachments(const ClearDesc* clearDescs, uint32_t clearDescNum, const Rect* rects, uint32_t rectNum)
{
    D3D12_RECT* rectsD3D12 = STACK_ALLOC(D3D12_RECT, rectNum);
    ConvertRects(rectsD3D12, rects, rectNum);

    for (uint32_t i = 0; i < clearDescNum; i++)
    {
        if (AttachmentContentType::COLOR == clearDescs[i].attachmentContentType)
            m_GraphicsCommandList->ClearRenderTargetView(m_RenderTargets[clearDescs[i].colorAttachmentIndex], &clearDescs[i].value.color32f.x, rectNum, rectsD3D12);
        else if (m_DepthStencil.ptr)
        {
            D3D12_CLEAR_FLAGS clearFlags = (D3D12_CLEAR_FLAGS)0;
            switch (clearDescs[i].attachmentContentType)
            {
            case AttachmentContentType::DEPTH:
                clearFlags = D3D12_CLEAR_FLAG_DEPTH;
                break;
            case AttachmentContentType::STENCIL:
                clearFlags = D3D12_CLEAR_FLAG_STENCIL;
                break;
            case AttachmentContentType::DEPTH_STENCIL:
                clearFlags = D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL;
                break;
            }

            m_GraphicsCommandList->ClearDepthStencilView(m_DepthStencil, clearFlags, clearDescs[i].value.depthStencil.depth, clearDescs[i].value.depthStencil.stencil, rectNum, rectsD3D12);
        }
    }
}

inline void CommandBufferD3D12::ClearStorageBuffer(const ClearStorageBufferDesc& clearDesc)
{
    DescriptorSetD3D12* descriptorSet = m_DescriptorSets[clearDesc.setIndexInPipelineLayout];
    DescriptorD3D12* resourceView = (DescriptorD3D12*)clearDesc.storageBuffer;
    const UINT clearValues[4] = { clearDesc.value, clearDesc.value , clearDesc.value, clearDesc.value };

    m_GraphicsCommandList->ClearUnorderedAccessViewUint(
    { descriptorSet->GetPointerGPU(clearDesc.rangeIndex, clearDesc.offsetInRange) },
    { resourceView->GetPointerCPU() },
        *resourceView,
        clearValues,
        0,
        nullptr);
}

inline void CommandBufferD3D12::ClearStorageTexture(const ClearStorageTextureDesc& clearDesc)
{
    DescriptorSetD3D12* descriptorSet = m_DescriptorSets[clearDesc.setIndexInPipelineLayout];
    DescriptorD3D12* resourceView = (DescriptorD3D12*)clearDesc.storageTexture;

    if (resourceView->IsIntegerFormat())
    {
        m_GraphicsCommandList->ClearUnorderedAccessViewUint(
        { descriptorSet->GetPointerGPU(clearDesc.rangeIndex, clearDesc.offsetInRange) },
        { resourceView->GetPointerCPU() },
            *resourceView,
            &clearDesc.value.color32ui.x,
            0,
            nullptr);
    }
    else
    {
        m_GraphicsCommandList->ClearUnorderedAccessViewFloat(
        { descriptorSet->GetPointerGPU(clearDesc.rangeIndex, clearDesc.offsetInRange) },
        { resourceView->GetPointerCPU() },
            *resourceView,
            &clearDesc.value.color32f.x,
            0,
            nullptr);
    }
}

inline void CommandBufferD3D12::BeginRendering(const AttachmentsDesc& attachmentsDesc)
{
    m_RenderTargetNum = attachmentsDesc.colors ? attachmentsDesc.colorNum : 0;

    uint32_t i = 0;
    for (; i < m_RenderTargetNum; i++)
    {
        const DescriptorD3D12& descriptor = *(DescriptorD3D12*)attachmentsDesc.colors[i];
        m_RenderTargets[i].ptr = descriptor.GetPointerCPU();
    }
    for (; i < (uint32_t)m_RenderTargets.size(); i++)
        m_RenderTargets[i].ptr = NULL;

    if (attachmentsDesc.depthStencil)
    {
        const DescriptorD3D12& descriptor = *(DescriptorD3D12*)attachmentsDesc.depthStencil;
        m_DepthStencil.ptr = descriptor.GetPointerCPU();
    }
    else
        m_DepthStencil.ptr = NULL;

    m_GraphicsCommandList->OMSetRenderTargets(m_RenderTargetNum, m_RenderTargets.data(), FALSE, m_DepthStencil.ptr ? &m_DepthStencil : nullptr);
}

inline void CommandBufferD3D12::SetVertexBuffers(uint32_t baseSlot, uint32_t bufferNum, const Buffer* const* buffers, const uint64_t* offsets)
{
    D3D12_VERTEX_BUFFER_VIEW* vertexBufferViews = STACK_ALLOC(D3D12_VERTEX_BUFFER_VIEW, bufferNum);

    for (uint32_t i = 0; i < bufferNum; i++)
    {
        if (buffers[i] != nullptr)
        {
            const BufferD3D12* buffer = (BufferD3D12*)buffers[i];
            uint64_t offset = offsets ? offsets[i] : 0;
            vertexBufferViews[i].BufferLocation = buffer->GetPointerGPU() + offset;
            vertexBufferViews[i].SizeInBytes = (UINT)(buffer->GetDesc().size - offset);
            vertexBufferViews[i].StrideInBytes = m_Pipeline->GetIAStreamStride(baseSlot + i);
        }
        else
        {
            vertexBufferViews[i].BufferLocation = 0;
            vertexBufferViews[i].SizeInBytes = 0;
            vertexBufferViews[i].StrideInBytes = 0;
        }
    }

    m_GraphicsCommandList->IASetVertexBuffers(baseSlot, bufferNum, vertexBufferViews);
}

inline void CommandBufferD3D12::SetIndexBuffer(const Buffer& buffer, uint64_t offset, IndexType indexType)
{
    const BufferD3D12& bufferD3D12 = (BufferD3D12&)buffer;

    D3D12_INDEX_BUFFER_VIEW indexBufferView;
    indexBufferView.BufferLocation = bufferD3D12.GetPointerGPU() + offset;
    indexBufferView.SizeInBytes = (UINT)(bufferD3D12.GetDesc().size - offset);
    indexBufferView.Format = indexType == IndexType::UINT16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;

    m_GraphicsCommandList->IASetIndexBuffer(&indexBufferView);
}

inline void CommandBufferD3D12::SetPipelineLayout(const PipelineLayout& pipelineLayout)
{
    const PipelineLayoutD3D12& pipelineLayoutD3D12 = (const PipelineLayoutD3D12&)pipelineLayout;

    if (m_PipelineLayout == &pipelineLayoutD3D12)
        return;

    m_PipelineLayout = &pipelineLayoutD3D12;
    m_IsGraphicsPipelineLayout = pipelineLayoutD3D12.IsGraphicsPipelineLayout();

    if (m_IsGraphicsPipelineLayout)
        m_GraphicsCommandList->SetGraphicsRootSignature(pipelineLayoutD3D12);
    else
        m_GraphicsCommandList->SetComputeRootSignature(pipelineLayoutD3D12);
}

inline void CommandBufferD3D12::SetPipeline(const Pipeline& pipeline)
{
    PipelineD3D12* pipelineD3D12 = (PipelineD3D12*)&pipeline;

    if (m_Pipeline == pipelineD3D12)
        return;

    pipelineD3D12->Bind(m_GraphicsCommandList, m_PrimitiveTopology);

    m_Pipeline = pipelineD3D12;
}

inline void CommandBufferD3D12::SetDescriptorPool(const DescriptorPool& descriptorPool)
{
    ((DescriptorPoolD3D12&)descriptorPool).Bind(m_GraphicsCommandList);
}

inline void CommandBufferD3D12::SetDescriptorSet(uint32_t setIndexInPipelineLayout, const DescriptorSet& descriptorSet, const uint32_t* dynamicConstantBufferOffsets)
{
    m_PipelineLayout->SetDescriptorSet(*m_GraphicsCommandList, m_IsGraphicsPipelineLayout, setIndexInPipelineLayout, descriptorSet, dynamicConstantBufferOffsets);
    m_DescriptorSets[setIndexInPipelineLayout] = (DescriptorSetD3D12*)&descriptorSet;
}

inline void CommandBufferD3D12::SetConstants(uint32_t pushConstantRangeIndex, const void* data, uint32_t size)
{
    uint32_t rootParameterIndex = m_PipelineLayout->GetPushConstantsRootOffset(pushConstantRangeIndex);
    uint32_t constantNum = size / 4;

    if (m_IsGraphicsPipelineLayout)
        m_GraphicsCommandList->SetGraphicsRoot32BitConstants(rootParameterIndex, constantNum, data, 0);
    else
        m_GraphicsCommandList->SetComputeRoot32BitConstants(rootParameterIndex, constantNum, data, 0);
}

inline void CommandBufferD3D12::Draw(uint32_t vertexNum, uint32_t instanceNum, uint32_t baseVertex, uint32_t baseInstance)
{
    m_GraphicsCommandList->DrawInstanced(vertexNum, instanceNum, baseVertex, baseInstance);
}

inline void CommandBufferD3D12::DrawIndexed(uint32_t indexNum, uint32_t instanceNum, uint32_t baseIndex, uint32_t baseVertex, uint32_t baseInstance)
{
    m_GraphicsCommandList->DrawIndexedInstanced(indexNum, instanceNum, baseIndex, baseVertex, baseInstance);
}

inline void CommandBufferD3D12::DrawIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride)
{
    m_GraphicsCommandList->ExecuteIndirect(m_Device.GetDrawCommandSignature(stride), drawNum, (BufferD3D12&)buffer, offset, nullptr, 0);
}

inline void CommandBufferD3D12::DrawIndexedIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride)
{
    m_GraphicsCommandList->ExecuteIndirect(m_Device.GetDrawIndexedCommandSignature(stride), drawNum, (BufferD3D12&)buffer, offset, nullptr, 0);
}

inline void CommandBufferD3D12::CopyBuffer(Buffer& dstBuffer, uint64_t dstOffset, const Buffer& srcBuffer, uint64_t srcOffset, uint64_t size)
{
    if (size == WHOLE_SIZE)
        size = ((BufferD3D12&)srcBuffer).GetDesc().size;

    m_GraphicsCommandList->CopyBufferRegion((BufferD3D12&)dstBuffer, dstOffset, (BufferD3D12&)srcBuffer, srcOffset, size);
}

inline void CommandBufferD3D12::CopyTexture(Texture& dstTexture, const TextureRegionDesc* dstRegion, const Texture& srcTexture, const TextureRegionDesc* srcRegion)
{
    TextureD3D12& dstTextureD3D12 = (TextureD3D12&)dstTexture;
    TextureD3D12& srcTextureD3D12 = (TextureD3D12&)srcTexture;

    if (!dstRegion || !srcRegion)
    {
        m_GraphicsCommandList->CopyResource(dstTextureD3D12, srcTextureD3D12);
    }
    else
    {
        D3D12_TEXTURE_COPY_LOCATION dstTextureCopyLocation = {
            dstTextureD3D12, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, dstTextureD3D12.GetSubresourceIndex(dstRegion->arrayOffset, dstRegion->mipOffset) };

        D3D12_TEXTURE_COPY_LOCATION srcTextureCopyLocation = {
            srcTextureD3D12, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, srcTextureD3D12.GetSubresourceIndex(srcRegion->arrayOffset, srcRegion->mipOffset) };

        const uint16_t size[3] = {
            srcRegion->width == WHOLE_SIZE ? srcTextureD3D12.GetSize(0, srcRegion->mipOffset) : srcRegion->width,
            srcRegion->height == WHOLE_SIZE ? srcTextureD3D12.GetSize(1, srcRegion->mipOffset) : srcRegion->height,
            srcRegion->depth == WHOLE_SIZE ? srcTextureD3D12.GetSize(2, srcRegion->mipOffset) : srcRegion->depth
        };
        D3D12_BOX box = { srcRegion->x, srcRegion->y, srcRegion->z, uint16_t(srcRegion->x + size[0]), uint16_t(srcRegion->y + size[1]), uint16_t(srcRegion->z + size[2]) };

        m_GraphicsCommandList->CopyTextureRegion(&dstTextureCopyLocation, dstRegion->x, dstRegion->y, dstRegion->z, &srcTextureCopyLocation, &box);
    }
}

inline void CommandBufferD3D12::UploadBufferToTexture(Texture& dstTexture, const TextureRegionDesc& dstRegionDesc, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayoutDesc)
{
    TextureD3D12& dstTextureD3D12 = (TextureD3D12&)dstTexture;
    D3D12_TEXTURE_COPY_LOCATION dstTextureCopyLocation = {
        dstTextureD3D12, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, dstTextureD3D12.GetSubresourceIndex(dstRegionDesc.arrayOffset, dstRegionDesc.mipOffset) };

    const uint16_t size[3] = {
        dstRegionDesc.width == WHOLE_SIZE ? dstTextureD3D12.GetSize(0, dstRegionDesc.mipOffset) : dstRegionDesc.width,
        dstRegionDesc.height == WHOLE_SIZE ? dstTextureD3D12.GetSize(1, dstRegionDesc.mipOffset) : dstRegionDesc.height,
        dstRegionDesc.depth == WHOLE_SIZE ? dstTextureD3D12.GetSize(2, dstRegionDesc.mipOffset) : dstRegionDesc.depth
    };

    D3D12_TEXTURE_COPY_LOCATION srcTextureCopyLocation;
    srcTextureCopyLocation.pResource = (BufferD3D12&)srcBuffer;
    srcTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcTextureCopyLocation.PlacedFootprint.Offset = srcDataLayoutDesc.offset;
    srcTextureCopyLocation.PlacedFootprint.Footprint.Format = GetDxgiFormat(dstTextureD3D12.GetDesc().format).typeless;
    srcTextureCopyLocation.PlacedFootprint.Footprint.Width = size[0];
    srcTextureCopyLocation.PlacedFootprint.Footprint.Height = size[1];
    srcTextureCopyLocation.PlacedFootprint.Footprint.Depth = size[2];
    srcTextureCopyLocation.PlacedFootprint.Footprint.RowPitch = srcDataLayoutDesc.rowPitch;

    D3D12_BOX box = { dstRegionDesc.x, dstRegionDesc.y, dstRegionDesc.z, uint16_t(dstRegionDesc.x + size[0]), uint16_t(dstRegionDesc.y + size[1]), uint16_t(dstRegionDesc.z + size[2]) };

    m_GraphicsCommandList->CopyTextureRegion(&dstTextureCopyLocation, dstRegionDesc.x, dstRegionDesc.y, dstRegionDesc.z, &srcTextureCopyLocation, &box);
}

inline void CommandBufferD3D12::ReadbackTextureToBuffer(Buffer& dstBuffer, TextureDataLayoutDesc& dstDataLayoutDesc, const Texture& srcTexture, const TextureRegionDesc& srcRegionDesc)
{
    TextureD3D12& srcTextureD3D12 = (TextureD3D12&)srcTexture;
    D3D12_TEXTURE_COPY_LOCATION srcTextureCopyLocation = {
        srcTextureD3D12, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, srcTextureD3D12.GetSubresourceIndex(srcRegionDesc.arrayOffset, srcRegionDesc.mipOffset) };

    D3D12_TEXTURE_COPY_LOCATION dstTextureCopyLocation;
    dstTextureCopyLocation.pResource = (BufferD3D12&)dstBuffer;
    dstTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dstTextureCopyLocation.PlacedFootprint.Offset = dstDataLayoutDesc.offset;
    dstTextureCopyLocation.PlacedFootprint.Footprint.Format = GetDxgiFormat(srcTextureD3D12.GetDesc().format).typeless;
    dstTextureCopyLocation.PlacedFootprint.Footprint.Width = srcRegionDesc.width;
    dstTextureCopyLocation.PlacedFootprint.Footprint.Height = srcRegionDesc.height;
    dstTextureCopyLocation.PlacedFootprint.Footprint.Depth = srcRegionDesc.depth;
    dstTextureCopyLocation.PlacedFootprint.Footprint.RowPitch = dstDataLayoutDesc.rowPitch;

    const uint16_t size[3] = {
        srcRegionDesc.width == WHOLE_SIZE ? srcTextureD3D12.GetSize(0, srcRegionDesc.mipOffset) : srcRegionDesc.width,
        srcRegionDesc.height == WHOLE_SIZE ? srcTextureD3D12.GetSize(1, srcRegionDesc.mipOffset) : srcRegionDesc.height,
        srcRegionDesc.depth == WHOLE_SIZE ? srcTextureD3D12.GetSize(2, srcRegionDesc.mipOffset) : srcRegionDesc.depth
    };
    D3D12_BOX box = { srcRegionDesc.x, srcRegionDesc.y, srcRegionDesc.z, uint16_t(srcRegionDesc.x + size[0]), uint16_t(srcRegionDesc.y + size[1]), uint16_t(srcRegionDesc.z + size[2]) };

    m_GraphicsCommandList->CopyTextureRegion(&dstTextureCopyLocation, 0, 0, 0, &srcTextureCopyLocation, &box);
}

inline void CommandBufferD3D12::Dispatch(uint32_t x, uint32_t y, uint32_t z)
{
    m_GraphicsCommandList->Dispatch(x, y, z);
}

inline void CommandBufferD3D12::DispatchIndirect(const Buffer& buffer, uint64_t offset)
{
    m_GraphicsCommandList->ExecuteIndirect(m_Device.GetDispatchCommandSignature(), 1, (BufferD3D12&)buffer, offset, nullptr, 0);
}

inline void CommandBufferD3D12::PipelineBarrier(const TransitionBarrierDesc* transitionBarriers, const AliasingBarrierDesc* aliasingBarriers, BarrierDependency dependency)
{
    MaybeUnused(dependency);

    uint32_t barrierNum = 0;
    if (transitionBarriers)
    {
        barrierNum += transitionBarriers->bufferNum;
        for (uint16_t i = 0; i < transitionBarriers->textureNum; i++)
        {
            const auto& barrierDesc = transitionBarriers->textures[i];
            const TextureD3D12& texture = *(TextureD3D12*)barrierDesc.texture;
            const TextureDesc& textureDesc = texture.GetDesc();
            const Dim_t arraySize = barrierDesc.arraySize == REMAINING_ARRAY_LAYERS ? textureDesc.arraySize : barrierDesc.arraySize;
            const Mip_t mipNum = barrierDesc.mipNum == REMAINING_MIP_LEVELS ? textureDesc.mipNum : barrierDesc.mipNum;
            if (barrierDesc.arrayOffset == 0 &&
                barrierDesc.arraySize == REMAINING_ARRAY_LAYERS &&
                barrierDesc.mipOffset == 0 &&
                barrierDesc.mipNum == REMAINING_MIP_LEVELS)
                barrierNum++;
            else
                barrierNum += arraySize * mipNum;
        }
    }
    if (aliasingBarriers)
    {
        barrierNum += aliasingBarriers->bufferNum;
        barrierNum += aliasingBarriers->textureNum;
    }

    if (!barrierNum)
        return;

    D3D12_RESOURCE_BARRIER* resourceBarriers = STACK_ALLOC(D3D12_RESOURCE_BARRIER, barrierNum);
    memset(resourceBarriers, 0, sizeof(D3D12_RESOURCE_BARRIER) * barrierNum);

    D3D12_RESOURCE_BARRIER* ptr = resourceBarriers;
    if (transitionBarriers) // UAV and transitions barriers
    {
        for (uint32_t i = 0; i < transitionBarriers->bufferNum; i++)
        {
            const auto& barrierDesc = transitionBarriers->buffers[i];
            AddResourceBarrier(*((BufferD3D12*)barrierDesc.buffer), barrierDesc.prevAccess, barrierDesc.nextAccess, *ptr++, 0);
        }

        for (uint32_t i = 0; i < transitionBarriers->textureNum; i++)
        {
            const auto& barrierDesc = transitionBarriers->textures[i];
            const TextureD3D12& texture = *(TextureD3D12*)barrierDesc.texture;
            const TextureDesc& textureDesc = texture.GetDesc();
            const Dim_t arraySize = barrierDesc.arraySize == REMAINING_ARRAY_LAYERS ? textureDesc.arraySize : barrierDesc.arraySize;
            const Mip_t mipNum = barrierDesc.mipNum == REMAINING_MIP_LEVELS ? textureDesc.mipNum : barrierDesc.mipNum;
            if (barrierDesc.arrayOffset == 0 &&
                barrierDesc.arraySize == REMAINING_ARRAY_LAYERS &&
                barrierDesc.mipOffset == 0 &&
                barrierDesc.mipNum == REMAINING_MIP_LEVELS)
            {
                AddResourceBarrier(texture, barrierDesc.prevState.acessBits, barrierDesc.nextState.acessBits, *ptr++, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
            }
            else
            {
                for (Dim_t arrayOffset = barrierDesc.arrayOffset; arrayOffset < barrierDesc.arrayOffset + arraySize; arrayOffset++)
                {
                    for (Mip_t mipOffset = barrierDesc.mipOffset; mipOffset < barrierDesc.mipOffset + mipNum; mipOffset++)
                    {
                        uint32_t subresource = texture.GetSubresourceIndex(arrayOffset, mipOffset);
                        AddResourceBarrier(texture, barrierDesc.prevState.acessBits, barrierDesc.nextState.acessBits, *ptr++, subresource);
                    }
                }
            }
        }
    }

    if (aliasingBarriers)
    {
        for (uint32_t i = 0; i < aliasingBarriers->bufferNum; i++)
        {
            D3D12_RESOURCE_BARRIER& barrier = *ptr++;
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
            barrier.Aliasing.pResourceBefore = *((BufferD3D12*)aliasingBarriers->buffers[i].before);
            barrier.Aliasing.pResourceAfter = *((BufferD3D12*)aliasingBarriers->buffers[i].after);
        }

        for (uint32_t i = 0; i < aliasingBarriers->textureNum; i++)
        {
            D3D12_RESOURCE_BARRIER& barrier = *ptr++;
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
            barrier.Aliasing.pResourceBefore = *((TextureD3D12*)aliasingBarriers->textures[i].before);
            barrier.Aliasing.pResourceAfter = *((TextureD3D12*)aliasingBarriers->textures[i].after);
        }
    }

    m_GraphicsCommandList->ResourceBarrier(barrierNum, resourceBarriers);
}

inline void CommandBufferD3D12::BeginQuery(const QueryPool& queryPool, uint32_t offset)
{
    const QueryPoolD3D12& queryPoolD3D12 = (QueryPoolD3D12&)queryPool;
    m_GraphicsCommandList->BeginQuery(queryPoolD3D12, queryPoolD3D12.GetType(), offset);
}

inline void CommandBufferD3D12::EndQuery(const QueryPool& queryPool, uint32_t offset)
{
    const QueryPoolD3D12& queryPoolD3D12 = (QueryPoolD3D12&)queryPool;
    m_GraphicsCommandList->EndQuery(queryPoolD3D12, queryPoolD3D12.GetType(), offset);
}

inline void CommandBufferD3D12::CopyQueries(const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& buffer, uint64_t alignedBufferOffset)
{
    const QueryPoolD3D12& queryPoolD3D12 = (QueryPoolD3D12&)queryPool;
    const BufferD3D12& bufferD3D12 = (BufferD3D12&)buffer;

    // WAR: QueryHeap uses a readback buffer for QueryType::ACCELERATION_STRUCTURE_COMPACTED_SIZE
    if (queryPoolD3D12.GetType() == (D3D12_QUERY_TYPE)-1)
    {
        const uint64_t srcOffset = offset * queryPoolD3D12.GetQuerySize();
        const uint64_t size = num * queryPoolD3D12.GetQuerySize();
        m_GraphicsCommandList->CopyBufferRegion(bufferD3D12, alignedBufferOffset, queryPoolD3D12.GetReadbackBuffer(), srcOffset, size);
        return;
    }

    m_GraphicsCommandList->ResolveQueryData(queryPoolD3D12, queryPoolD3D12.GetType(), offset, num, bufferD3D12, alignedBufferOffset);
}

inline void CommandBufferD3D12::BeginAnnotation(const char* name)
{
    size_t len = strlen(name) + 1;
    wchar_t* s = STACK_ALLOC(wchar_t, len);
    ConvertCharToWchar(name, s, len);

    PIXBeginEvent(m_GraphicsCommandList, PIX_COLOR_DEFAULT, s);
}

inline void CommandBufferD3D12::EndAnnotation()
{
    PIXEndEvent(m_GraphicsCommandList);
}

inline void CommandBufferD3D12::BuildTopLevelAccelerationStructure(uint32_t instanceNum, const Buffer& buffer, uint64_t bufferOffset,
    AccelerationStructureBuildBits flags, AccelerationStructure& dst, Buffer& scratch, uint64_t scratchOffset)
{
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
    desc.DestAccelerationStructureData = ((AccelerationStructureD3D12&)dst).GetHandle();
    desc.ScratchAccelerationStructureData = ((BufferD3D12&)scratch).GetPointerGPU() + scratchOffset;
    desc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    desc.Inputs.Flags = GetAccelerationStructureBuildFlags(flags);
    desc.Inputs.NumDescs = instanceNum;
    desc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

    desc.Inputs.InstanceDescs = ((BufferD3D12&)buffer).GetPointerGPU() + bufferOffset;

    m_GraphicsCommandList4->BuildRaytracingAccelerationStructure(&desc, 0, nullptr);
}

inline void CommandBufferD3D12::BuildBottomLevelAccelerationStructure(uint32_t geometryObjectNum, const GeometryObject* geometryObjects,
    AccelerationStructureBuildBits flags, AccelerationStructure& dst, Buffer& scratch, uint64_t scratchOffset)
{
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
    desc.DestAccelerationStructureData = ((AccelerationStructureD3D12&)dst).GetHandle();
    desc.ScratchAccelerationStructureData = ((BufferD3D12&)scratch).GetPointerGPU() + scratchOffset;
    desc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    desc.Inputs.Flags = GetAccelerationStructureBuildFlags(flags);
    desc.Inputs.NumDescs = geometryObjectNum;
    desc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

    Vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs(geometryObjectNum, m_Device.GetStdAllocator());
    ConvertGeometryDescs(&geometryDescs[0], geometryObjects, geometryObjectNum);
    desc.Inputs.pGeometryDescs = &geometryDescs[0];

    m_GraphicsCommandList4->BuildRaytracingAccelerationStructure(&desc, 0, nullptr);
}

inline void CommandBufferD3D12::UpdateTopLevelAccelerationStructure(uint32_t instanceNum, const Buffer& buffer, uint64_t bufferOffset,
    AccelerationStructureBuildBits flags, AccelerationStructure& dst, AccelerationStructure& src, Buffer& scratch, uint64_t scratchOffset)
{
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
    desc.DestAccelerationStructureData = ((AccelerationStructureD3D12&)dst).GetHandle();
    desc.SourceAccelerationStructureData = ((AccelerationStructureD3D12&)src).GetHandle();
    desc.ScratchAccelerationStructureData = ((BufferD3D12&)scratch).GetPointerGPU() + scratchOffset;
    desc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    desc.Inputs.Flags = GetAccelerationStructureBuildFlags(flags) | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
    desc.Inputs.NumDescs = instanceNum;
    desc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

    desc.Inputs.InstanceDescs = ((BufferD3D12&)buffer).GetPointerGPU() + bufferOffset;

    m_GraphicsCommandList4->BuildRaytracingAccelerationStructure(&desc, 0, nullptr);
}

inline void CommandBufferD3D12::UpdateBottomLevelAccelerationStructure(uint32_t geometryObjectNum, const GeometryObject* geometryObjects,
    AccelerationStructureBuildBits flags, AccelerationStructure& dst, AccelerationStructure& src, Buffer& scratch, uint64_t scratchOffset)
{
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
    desc.DestAccelerationStructureData = ((AccelerationStructureD3D12&)dst).GetHandle();
    desc.SourceAccelerationStructureData = ((AccelerationStructureD3D12&)src).GetHandle();
    desc.ScratchAccelerationStructureData = ((BufferD3D12&)scratch).GetPointerGPU() + scratchOffset;
    desc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    desc.Inputs.Flags = GetAccelerationStructureBuildFlags(flags) | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
    desc.Inputs.NumDescs = geometryObjectNum;
    desc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

    Vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs(geometryObjectNum, m_Device.GetStdAllocator());
    ConvertGeometryDescs(&geometryDescs[0], geometryObjects, geometryObjectNum);
    desc.Inputs.pGeometryDescs = &geometryDescs[0];

    m_GraphicsCommandList4->BuildRaytracingAccelerationStructure(&desc, 0, nullptr);
}

inline void CommandBufferD3D12::CopyAccelerationStructure(AccelerationStructure& dst, AccelerationStructure& src, CopyMode copyMode)
{
    m_GraphicsCommandList4->CopyRaytracingAccelerationStructure(((AccelerationStructureD3D12&)dst).GetHandle(), ((AccelerationStructureD3D12&)src).GetHandle(), GetCopyMode(copyMode));
}

inline void CommandBufferD3D12::WriteAccelerationStructureSize(const AccelerationStructure* const* accelerationStructures, uint32_t accelerationStructureNum, QueryPool& queryPool, uint32_t queryOffset)
{
    D3D12_GPU_VIRTUAL_ADDRESS* virtualAddresses = ALLOCATE_SCRATCH(m_Device, D3D12_GPU_VIRTUAL_ADDRESS, accelerationStructureNum);
    for (uint32_t i = 0; i < accelerationStructureNum; i++)
        virtualAddresses[i] = ((AccelerationStructureD3D12&)accelerationStructures[i]).GetHandle();

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC postbuildInfo = {};
    postbuildInfo.InfoType = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE;
    postbuildInfo.DestBuffer = ((QueryPoolD3D12&)queryPool).GetReadbackBuffer()->GetGPUVirtualAddress() + queryOffset;

    m_GraphicsCommandList4->EmitRaytracingAccelerationStructurePostbuildInfo(&postbuildInfo, accelerationStructureNum, virtualAddresses);

    FREE_SCRATCH(m_Device, virtualAddresses, accelerationStructureNum);
}

inline void CommandBufferD3D12::DispatchRays(const DispatchRaysDesc& dispatchRaysDesc)
{
    D3D12_DISPATCH_RAYS_DESC desc = {};

    desc.RayGenerationShaderRecord.StartAddress = (*(BufferD3D12*)dispatchRaysDesc.raygenShader.buffer).GetPointerGPU() + dispatchRaysDesc.raygenShader.offset;
    desc.RayGenerationShaderRecord.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

    if (dispatchRaysDesc.missShaders.buffer)
    {
        desc.MissShaderTable.StartAddress = (*(BufferD3D12*)dispatchRaysDesc.missShaders.buffer).GetPointerGPU() + dispatchRaysDesc.missShaders.offset;
        desc.MissShaderTable.SizeInBytes = dispatchRaysDesc.missShaders.size;
        desc.MissShaderTable.StrideInBytes = dispatchRaysDesc.missShaders.stride;
    }

    if (dispatchRaysDesc.hitShaderGroups.buffer)
    {
        desc.HitGroupTable.StartAddress = (*(BufferD3D12*)dispatchRaysDesc.hitShaderGroups.buffer).GetPointerGPU() + dispatchRaysDesc.hitShaderGroups.offset;
        desc.HitGroupTable.SizeInBytes = dispatchRaysDesc.hitShaderGroups.size;
        desc.HitGroupTable.StrideInBytes = dispatchRaysDesc.hitShaderGroups.stride;
    }

    if (dispatchRaysDesc.callableShaders.buffer)
    {
        desc.CallableShaderTable.StartAddress = (*(BufferD3D12*)dispatchRaysDesc.callableShaders.buffer).GetPointerGPU() + dispatchRaysDesc.callableShaders.offset;
        desc.CallableShaderTable.SizeInBytes = dispatchRaysDesc.callableShaders.size;
        desc.CallableShaderTable.StrideInBytes = dispatchRaysDesc.callableShaders.stride;
    }

    desc.Width = dispatchRaysDesc.width;
    desc.Height = dispatchRaysDesc.height;
    desc.Depth = dispatchRaysDesc.depth;

    m_GraphicsCommandList4->DispatchRays(&desc);
}

inline void CommandBufferD3D12::DispatchMeshTasks(uint32_t x, uint32_t y, uint32_t z)
{
    m_GraphicsCommandList6->DispatchMesh(x, y, z);
}

#include "CommandBufferD3D12.hpp"
