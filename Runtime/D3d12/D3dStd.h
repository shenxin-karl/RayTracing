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
#include <glm/glm.hpp>
#include <variant>
#include <glm/ext/matrix_transform.hpp>

#define ENABLE_D3D_11 0

#ifndef ENABLE_D3D_COMPUTE_QUEUE
    #define ENABLE_D3D_COMPUTE_QUEUE 0
#endif

#ifndef ENABLE_D3D_11
    #define ENABLE_D3D_11 0
#endif

namespace dx {

// clang-format off
// global
#if ENABLE_D3D_11
constexpr inline D3D_FEATURE_LEVEL KD3D_FEATURE_LEVEL       = D3D_FEATURE_LEVEL_11_0;
using NativeDevice                                          = ID3D12Device2;
using NativeCommandList                                     = ID3D12GraphicsCommandList;
#define ENABLE_RAY_TRACING 0
#else
constexpr inline D3D_FEATURE_LEVEL KD3D_FEATURE_LEVEL       = D3D_FEATURE_LEVEL_12_0;
using NativeDevice                                          = ID3D12Device6;
using NativeCommandList                                     = ID3D12GraphicsCommandList6;
#define ENABLE_RAY_TRACING 1
#endif

constexpr inline std::size_t kMaxRootParameter              = 16;	
constexpr inline std::size_t kMaxDescriptor                 = 256;	
constexpr inline std::size_t kMaxDescriptorInRootParameter  = 256;
constexpr inline std::size_t kDynamicDescriptorMaxView		= 256;
constexpr inline std::size_t kDynamicDescriptorMaxSampler	= 32;

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

class BottomLevelAS;
class TopLevelAS;
class IASBuilder;
class SyncASBuilder;
class AsyncASBuilder;

class RTV;
class DSV;
class CBV;
class UAV;
class SRV;
class SAMPLER;

class Buffer;
class StaticBuffer;
class DynamicDescriptorHeap;

class TopLevelASGenerator;
class BottomLevelASGenerator;

class DescriptorHandleArray;
class BindlessCollection;

namespace WRL = Microsoft::WRL;

enum class ContextType {
    eGraphics = 0,
    eCompute = 1,
};

// clang-format off

/// top acceleration structure Instance flag
enum class RayTracingInstanceFlags : uint16_t {
    eNone                           = D3D12_RAYTRACING_INSTANCE_FLAG_NONE,
    eTriangleCullDisable            = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE,
    eTriangleFrontCounterClockWise  = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE,
    eForceOpaque                    = D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_OPAQUE,
    eForceNonOpaque                 = D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_NON_OPAQUE,
};
ENUM_FLAGS(RayTracingInstanceFlags)
// clang-format off


struct ASInstance {
    ID3D12Resource *pBottomLevelAs = nullptr;
    glm::mat4x4 transform = glm::identity<glm::mat4x4>();
    uint32_t instanceID = 0;
    uint32_t hitGroupIndex = 0;
    uint16_t instanceMask = 0xff;
    RayTracingInstanceFlags instanceFlag = RayTracingInstanceFlags::eNone;
public:
    bool IsValid() const {
        return pBottomLevelAs != nullptr;
    }
};

struct DWParam {
    DWParam(float f) : Float(f) {
    }
    DWParam(uint32_t u) : Uint(u) {
    }
    DWParam(int32_t i) : Int(i) {
    }
    void operator=(float f) {
        Float = f;
    }
    void operator=(uint32_t u) {
        Uint = u;
    }
    void operator=(int32_t i) {
        Int = i;
    }
    union {
        float Float;
        uint32_t Uint;
        int32_t Int;
    };
    static DWParam Zero;
};

inline DWParam DWParam::Zero = {0.f};

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

template<typename T>
Inline(2) constexpr size_t SizeofInUint32(const T &obj) {
    return (sizeof(T) - 1) / sizeof(uint32_t) + 1;
}

template<typename T>
Inline(2) constexpr size_t SizeofInUint32() {
    return (sizeof(T) - 1) / sizeof(uint32_t) + 1;
}

Inline(2) CD3DX12_STATIC_SAMPLER_DESC GetPointWrapStaticSampler(UINT shaderRegister) {
    return CD3DX12_STATIC_SAMPLER_DESC(shaderRegister,
        D3D12_FILTER_MIN_MAG_MIP_POINT,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP);
}

Inline(2) CD3DX12_STATIC_SAMPLER_DESC GetPointClampStaticSampler(UINT shaderRegister) {
    return CD3DX12_STATIC_SAMPLER_DESC(shaderRegister,
        D3D12_FILTER_MIN_MAG_MIP_POINT,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
}

Inline(2) CD3DX12_STATIC_SAMPLER_DESC GetLinearWrapStaticSampler(UINT shaderRegister) {
    return CD3DX12_STATIC_SAMPLER_DESC(shaderRegister,
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP);
}

Inline(2) CD3DX12_STATIC_SAMPLER_DESC GetLinearClampStaticSampler(UINT shaderRegister) {
    return CD3DX12_STATIC_SAMPLER_DESC(shaderRegister,
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
}

Inline(2) CD3DX12_STATIC_SAMPLER_DESC GetAnisotropicWrapStaticSampler(UINT shaderRegister) {
    return CD3DX12_STATIC_SAMPLER_DESC(shaderRegister,
        D3D12_FILTER_ANISOTROPIC,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP);
}

Inline(2) CD3DX12_STATIC_SAMPLER_DESC GetAnisotropicClampStaticSampler(UINT shaderRegister) {
    return CD3DX12_STATIC_SAMPLER_DESC(shaderRegister,
        D3D12_FILTER_ANISOTROPIC,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
}

Inline(2) CD3DX12_STATIC_SAMPLER_DESC GetLinearShadowCompareStaticSampler(UINT shaderRegister) {
    return CD3DX12_STATIC_SAMPLER_DESC(shaderRegister,       // shaderRegister
        D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,    // filter
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,                   // addressU
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,                   // addressV
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,                   // addressW
        0.0f,                                                // mipLODBias
        16,                                                  // maxAnisotropy
        D3D12_COMPARISON_FUNC_LESS_EQUAL,
        D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE);
}

Inline(2) CD3DX12_STATIC_SAMPLER_DESC GetPointShadowCompareStaticSampler(UINT shaderRegister) {
    return CD3DX12_STATIC_SAMPLER_DESC(shaderRegister,    // shaderRegister
        D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT,        // filter
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,                // addressU
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,                // addressV
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,                // addressW
        0.0f,                                             // mipLODBias
        16,                                               // maxAnisotropy
        D3D12_COMPARISON_FUNC_LESS_EQUAL,
        D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE);
}

Inline(2) std::array<CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplerArray() {
    return {
        GetPointWrapStaticSampler(0),
        GetPointClampStaticSampler(1),
        GetLinearWrapStaticSampler(2),
        GetLinearClampStaticSampler(3),
        GetAnisotropicWrapStaticSampler(4),
        GetAnisotropicClampStaticSampler(5),
    };
};

}    // namespace dx

template<>
struct std::hash<D3D12_CPU_DESCRIPTOR_HANDLE> {
    using argument_type = D3D12_CPU_DESCRIPTOR_HANDLE;
    using result_type = size_t;

    [[nodiscard]]
    result_type
    operator()(const argument_type &handle) const noexcept {
        return std::hash<size_t>{}(handle.ptr);
    }
};

inline bool operator==(D3D12_CPU_DESCRIPTOR_HANDLE lhs, D3D12_CPU_DESCRIPTOR_HANDLE rhs) {
    return lhs.ptr == rhs.ptr;
}