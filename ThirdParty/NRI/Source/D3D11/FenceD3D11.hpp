// © 2021 NVIDIA Corporation

#pragma region [  Core  ]

static uint64_t NRI_CALL GetFenceValue(Fence& fence)
{
    return ((FenceD3D11&)fence).GetFenceValue();
}

static void NRI_CALL QueueSignal(CommandQueue& commandQueue, Fence& fence, uint64_t value)
{
    return ((FenceD3D11&)fence).QueueSignal((CommandQueueD3D11&)commandQueue, value);
}

static void NRI_CALL QueueWait(CommandQueue& commandQueue, Fence& fence, uint64_t value)
{
    return ((FenceD3D11&)fence).QueueWait((CommandQueueD3D11&)commandQueue, value);
}

static void NRI_CALL Wait(Fence& fence, uint64_t value)
{
    ((FenceD3D11&)fence).Wait(value);
}

static void NRI_CALL SetFenceDebugName(Fence& fence, const char* name)
{
    ((FenceD3D11&)fence).SetDebugName(name);
}

#pragma endregion

Define_Core_Fence_PartiallyFillFunctionTable(D3D11)
