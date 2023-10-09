#pragma once
#include <Windows.h>
#include <source_location>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3D12MemAlloc.h>
#include <dxcapi.h>

#include "Foundation/Exception.h"
#include "Foundation/PreprocessorDirectives.h"
#include "Foundation/NonCopyable.h"

#include "d3dx12.h"
#include "FormatHelper.hpp"

#include <optional>
#include <span>
#include <queue>
#include <bitset>
#include <array>

#ifndef ENABLE_D3D_COMPUTE_QUEUE
    #define ENABLE_D3D_COMPUTE_QUEUE 0
#endif

namespace dx {

// clang-format off
// global config
constexpr inline D3D_FEATURE_LEVEL KD3D_FEATURE_LEVEL       = D3D_FEATURE_LEVEL_12_0;
using NativeDevice                                          = ID3D12Device;
using NativeCommandList                                     = ID3D12GraphicsCommandList6;
constexpr inline std::size_t kMaxRootParameter              = 16;	
constexpr inline std::size_t kMaxDescriptor                 = 256;	
constexpr inline std::size_t kMaxDescriptorInRootParameter  = 256;
// clang-format on

class Device;
class SwapChain;
class Fence;
class CommandListRing;
class UploadHeap;
class RootParameter;
class RootSignature;
class DescriptorAllocator;
class DescriptorHandle;
class DescriptorPage;
class DescriptorManager;
class Texture;

class Context;
class ComputeContext;
class GraphicsContext;

class CommandListPool;
class FrameResource;
class FrameResourceRing;

class RTV;
class DSV;
class CBV;
class UAV;
class SRV;
class SAMPLER;

class StaticBuffer;
class DynamicDescriptorHeap;

namespace WRL = Microsoft::WRL;

enum class ContextType {
    eGraphics = 0,
    eCompute = 1,
};

Inline(2) void ThrowIfFailed(HRESULT hr, const std::source_location &location = std::source_location::current()) {
    if (FAILED(hr)) {
        FormatAndLocation formatAndLocation{"d3d api error", location};
        Exception::Throw(formatAndLocation);
    }
}

template<typename T>
Inline(2) T *RVPtr(T &&val) {
    return &val;
}

// align val to the next multiple of alignment
template<std::integral T1, std::integral T2>
Inline(2) constexpr T1 AlignUp(T1 val, T2 alignment) {
    return (val + alignment - static_cast<T1>(1)) & ~(alignment - static_cast<T2>(1));
}

// align val to the previous multiple of alignment
template<std::integral T1, std::integral T2>
Inline(2) constexpr T1 AlignDown(T1 val, T2 alignment) {
    return val & ~(alignment - static_cast<T2>(1));
}

template<std::integral T1, std::integral T2>
Inline(2) constexpr T1 DivideRoundingUp(T1 a, T2 b) {
    return (a + b - static_cast<T2>(1)) / b;
}

Inline(2) constexpr size_t GetKByte(size_t num) {
    return num * 1024;
}

Inline(2) constexpr size_t GetMByte(size_t num) {
    return num * 1024 * 1024;
}

}    // namespace dx