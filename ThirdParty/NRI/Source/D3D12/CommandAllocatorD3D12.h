// © 2021 NVIDIA Corporation

#pragma once

struct ID3D12CommandAllocator;
enum D3D12_COMMAND_LIST_TYPE;

namespace nri
{

struct DeviceD3D12;

struct CommandAllocatorD3D12
{
    inline CommandAllocatorD3D12(DeviceD3D12& device)
        : m_Device(device)
    {}

    inline ~CommandAllocatorD3D12()
    {}

    inline operator ID3D12CommandAllocator*() const
    { return m_CommandAllocator.GetInterface(); }

    inline DeviceD3D12& GetDevice() const
    { return m_Device; }

    Result Create(const CommandQueue& commandQueue);

    //================================================================================================================
    // NRI
    //================================================================================================================

    inline void SetDebugName(const char* name)
    { SET_D3D_DEBUG_OBJECT_NAME(m_CommandAllocator, name); }

    Result CreateCommandBuffer(CommandBuffer*& commandBuffer);
    void Reset();

private:
    DeviceD3D12& m_Device;
    ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
    D3D12_COMMAND_LIST_TYPE m_CommandListType = D3D12_COMMAND_LIST_TYPE(-1);
};

}
