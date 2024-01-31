#include "FSR2Integration.h"
#include <limits>
#include <cmath>
#include <FidelityFX/host/backends/dx12/ffx_dx12.h>

#include "Components/Camera.h"
#include "D3d12/Device.h"
#include "D3d12/Texture.h"
#include "Foundation/GameTimer.h"
#include "Foundation/Logger.h"
#include "Renderer/GfxDevice.h"
#include "Renderer/RenderSetting.h"
#include "Renderer/RenderUtils/UserMarker.h"

FSR2Integration::FSR2Integration()
    : _scalePreset(),
      _mipBias(0),
      _sharpness(0),
      _useMask(false),
      _RCASSharpen(false),
      _maskMode(FSR2MaskMode::Disabled),
      _jitterIndex(0),
      _jitterPhaseCount(0),
      _contextDest() {
    SetScalePreset(FSR2ScalePreset::Quality);
}

void FSR2Integration::OnCreate() {
    GfxDevice *pGfxDevice = GfxDevice::GetInstance();
    size_t scratchBufferSize = ffxGetScratchMemorySizeDX12(FFX_FSR2_CONTEXT_COUNT);
    void *pScratchBuffer = new std::byte[scratchBufferSize];
    FfxDevice ffxDevice = ffxGetDeviceDX12(pGfxDevice->GetDevice()->GetNativeDevice());
    FfxErrorCode errorCode = ffxGetInterfaceDX12(&_contextDest.backendInterface,
        ffxDevice,
        pScratchBuffer,
        scratchBufferSize,
        FFX_FSR2_CONTEXT_COUNT);
    Assert(errorCode == FFX_OK);
}

void FSR2Integration::OnDestroy() {
    DestroyContext();
    if (_contextDest.backendInterface.scratchBuffer != nullptr) {
        delete[] _contextDest.backendInterface.scratchBuffer;
        _contextDest.backendInterface.scratchBuffer = nullptr;
    }
}

void FSR2Integration::OnResize(const ResolutionInfo &resolution) {
    DestroyContext();
    CreateContext(resolution);
    _jitterPhaseCount = ffxFsr2GetJitterPhaseCount(resolution.renderWidth, resolution.displayWidth);
}

static FfxSurfaceFormat ConvertToFfxFormat(DXGI_FORMAT format) {
    constexpr std::pair<DXGI_FORMAT, FfxSurfaceFormat> kFormatMap[] = {
        {DXGI_FORMAT_UNKNOWN, FFX_SURFACE_FORMAT_UNKNOWN},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS, FFX_SURFACE_FORMAT_R32G32B32A32_TYPELESS},
        {DXGI_FORMAT_R32G32B32A32_UINT, FFX_SURFACE_FORMAT_R32G32B32A32_UINT},
        {DXGI_FORMAT_R32G32B32A32_FLOAT, FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT},
        {DXGI_FORMAT_R16G16B16A16_FLOAT, FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT},
        {DXGI_FORMAT_R32G32_FLOAT, FFX_SURFACE_FORMAT_R32G32_FLOAT},
        {DXGI_FORMAT_R8_UINT, FFX_SURFACE_FORMAT_R8_UINT},
        {DXGI_FORMAT_R32_UINT, FFX_SURFACE_FORMAT_R32_UINT},
        {DXGI_FORMAT_R8G8B8A8_TYPELESS, FFX_SURFACE_FORMAT_R8G8B8A8_TYPELESS},
        {DXGI_FORMAT_R8G8B8A8_UNORM, FFX_SURFACE_FORMAT_R8G8B8A8_UNORM},
        {DXGI_FORMAT_R8G8B8A8_SNORM, FFX_SURFACE_FORMAT_R8G8B8A8_SNORM},
        {DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, FFX_SURFACE_FORMAT_R8G8B8A8_SRGB},
        {DXGI_FORMAT_R11G11B10_FLOAT, FFX_SURFACE_FORMAT_R11G11B10_FLOAT},
        {DXGI_FORMAT_R16G16_FLOAT, FFX_SURFACE_FORMAT_R16G16_FLOAT},
        {DXGI_FORMAT_R16G16_UINT, FFX_SURFACE_FORMAT_R16G16_UINT},
        {DXGI_FORMAT_R16_FLOAT, FFX_SURFACE_FORMAT_R16_FLOAT},
        {DXGI_FORMAT_R16_UINT, FFX_SURFACE_FORMAT_R16_UINT},
        {DXGI_FORMAT_R16_UNORM, FFX_SURFACE_FORMAT_R16_UNORM},
        {DXGI_FORMAT_R16_SNORM, FFX_SURFACE_FORMAT_R16_SNORM},
        {DXGI_FORMAT_R8_UNORM, FFX_SURFACE_FORMAT_R8_UNORM},
        {DXGI_FORMAT_R8G8_UNORM, FFX_SURFACE_FORMAT_R8G8_UNORM},
        {DXGI_FORMAT_R32_FLOAT, FFX_SURFACE_FORMAT_R32_FLOAT},
    };
    FfxSurfaceFormat ffxSurfaceFormat = FFX_SURFACE_FORMAT_UNKNOWN;
    for (auto &&[dxgiFormat, ffxFormat] : kFormatMap) {
        if (format == dxgiFormat) {
            ffxSurfaceFormat = ffxFormat;
        }
    }
    return ffxSurfaceFormat;
}

static FfxResource ConvertFfxResource(const dx::Texture *pTexture, std::wstring_view name) {
    FfxResource ffxResource = {};
    ffxResource.resource = pTexture->GetResource();
    ffxResource.description.type = FFX_RESOURCE_TYPE_TEXTURE2D;
    ffxResource.description.format = ConvertToFfxFormat(pTexture->GetFormat());
    ffxResource.description.width = pTexture->GetWidth();
    ffxResource.description.height = pTexture->GetHeight();
    ffxResource.description.depth = pTexture->GetDepthOrArraySize();
    ffxResource.description.mipCount = pTexture->GetMipCount();
    ffxResource.description.flags = FFX_RESOURCE_FLAGS_NONE;
    ffxResource.description.usage = FFX_RESOURCE_USAGE_READ_ONLY;
    D3D12_RESOURCE_FLAGS flags = pTexture->GetFlags();
    if (flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) {
        ffxResource.description.usage = FFX_RESOURCE_USAGE_RENDERTARGET;
    }
    if (flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) {
        ffxResource.description.usage = FFX_RESOURCE_USAGE_UAV;
    }
    if (flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) {
        ffxResource.description.format = ConvertToFfxFormat(
            dx::GetTypelessDepthTextureSRVFormat(pTexture->GetFormat()));
        ffxResource.description.usage = FFX_RESOURCE_USAGE_DEPTHTARGET;
    }
    ffxResource.state = FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ;
    std::memcpy(ffxResource.name, name.data(), std::min<size_t>(name.size(), FFX_RESOURCE_NAME_SIZE));
    Assert(ffxResource.description.format != FFX_SURFACE_FORMAT_UNKNOWN);
    return ffxResource;
}

void FSR2Integration::Execute(const FSR2ExecuteDesc &desc) {
    dx::ComputeContext *pComputeContext = desc.pComputeContext;
    UserMarker userMarker(pComputeContext, "FSR2");

    Assert(desc.pDepthTex->GetFormat() == DXGI_FORMAT_R32_TYPELESS);

    FfxFsr2DispatchDescription dispatchParameters = {};
    dispatchParameters.commandList = ffxGetCommandListDX12(pComputeContext->GetCommandList());

    pComputeContext->Transition(desc.pColorTex->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    dispatchParameters.color = ConvertFfxResource(desc.pColorTex, L"FSR2_Input_Color");

    pComputeContext->Transition(desc.pDepthTex->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    dispatchParameters.depth = ConvertFfxResource(desc.pDepthTex, L"FSR2_InputDepth");

    pComputeContext->Transition(desc.pMotionVectorTex->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    dispatchParameters.motionVectors = ConvertFfxResource(desc.pMotionVectorTex, L"FSR2_Input_MotionVectors");

    if (desc.pExposureTex != nullptr) {
        pComputeContext->Transition(desc.pExposureTex->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        dispatchParameters.exposure = ConvertFfxResource(desc.pExposureTex, L"FSR2_InputExposure");
    }

    pComputeContext->Transition(desc.pOutputTex->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    dispatchParameters.output = ConvertFfxResource(desc.pOutputTex, L"FSR2_Output");

    if (_maskMode != FSR2MaskMode::Disabled) {
        pComputeContext->Transition(desc.pReactiveMaskTex->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        dispatchParameters.reactive = ConvertFfxResource(desc.pReactiveMaskTex, L"FSR2_InputReactiveMap");
    }
    if (_useMask) {
        pComputeContext->Transition(desc.pCompositionMaskTex->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        dispatchParameters.transparencyAndComposition = ConvertFfxResource(desc.pCompositionMaskTex,
            L"FSR2_TransparencyAndCompositionMap");
    }

    float jitterX, jitterY;
    GetJitterOffset(jitterX, jitterY);
    dispatchParameters.jitterOffset.x = jitterX;
    dispatchParameters.jitterOffset.y = jitterY;
    dispatchParameters.motionVectorScale.x = desc.pColorTex->GetWidth();
    dispatchParameters.motionVectorScale.y = desc.pColorTex->GetHeight();
    dispatchParameters.reset = false;
    dispatchParameters.enableSharpening = _RCASSharpen;
    dispatchParameters.sharpness = _sharpness;

    // Cauldron keeps time in seconds, but FSR expects miliseconds
    dispatchParameters.frameTimeDelta = GameTimer::Get().GetDeltaTime() * 1000.f;

    dispatchParameters.preExposure = RenderSetting::Get().GetExposure();
    dispatchParameters.renderSize.width = desc.pColorTex->GetWidth();
    dispatchParameters.renderSize.height = desc.pColorTex->GetHeight();

    // Setup camera params as required
    dispatchParameters.cameraFovAngleVertical = glm::radians(desc.pCameraState->fov);
    if (RenderSetting::Get().GetReversedZ()) {
        dispatchParameters.cameraFar = desc.pCameraState->zNear;
        dispatchParameters.cameraNear = FLT_MAX;
    } else {
	    dispatchParameters.cameraFar = desc.pCameraState->zFar;
        dispatchParameters.cameraNear = desc.pCameraState->zNear;
    }

    FfxErrorCode errorCode = ffxFsr2ContextDispatch(_pContext.get(), &dispatchParameters);
    FFX_ASSERT(errorCode == FFX_OK);

    // FSR2 会修改 DescriptorHeaps. 重新绑定自己的
    pComputeContext->BindDescriptorHeaps();
}

static float CalculateMipBias(float upscalerRatio) {
    return std::log2f(1.f / upscalerRatio) - 1.f + std::numeric_limits<float>::epsilon();
}

void FSR2Integration::SetScalePreset(FSR2ScalePreset preset) {
    _scalePreset = preset;
    switch (preset) {
    case FSR2ScalePreset::Quality:
        _upscaleRatio = 1.5f;
        break;
    case FSR2ScalePreset::Balanced:
        _upscaleRatio = 1.7f;
        break;
    case FSR2ScalePreset::Performance:
        _upscaleRatio = 2.f;
        break;
    case FSR2ScalePreset::UltraPerformance:
        _upscaleRatio = 3.f;
        break;
    case FSR2ScalePreset::Custom:
    default:;
        break;
    }
    _mipBias = CalculateMipBias(_upscaleRatio);
}

auto FSR2Integration::GetResolutionInfo(size_t width, size_t height) const -> ResolutionInfo {
    ResolutionInfo resolution;
    resolution.renderWidth = static_cast<uint32_t>(std::ceil(static_cast<float>(width) / _upscaleRatio));
    resolution.renderHeight = static_cast<uint32_t>(std::ceil(static_cast<float>(height) / _upscaleRatio));
    resolution.displayWidth = width;
    resolution.displayHeight = height;
    return resolution;
}

void FSR2Integration::GetJitterOffset(float &jitterX, float &jitterY) const {
    ffxFsr2GetJitterOffset(&jitterX, &jitterY, _jitterIndex, _jitterPhaseCount);
}

void FSR2Integration::FfxMsgCallback(FfxMsgType type, const wchar_t *pMsg) {
    if (type == FFX_MESSAGE_TYPE_ERROR) {
        Logger::Error("FSR2_API_DEBUG_ERROR: {}", pMsg);
    } else if (type == FFX_MESSAGE_TYPE_WARNING) {
        Logger::Warning("FSR2_API_DEBUG_WARNING: {}", pMsg);
    }
}

void FSR2Integration::DestroyContext() {
    if (_pContext == nullptr) {
        return;
    }
    ffxFsr2ContextDestroy(_pContext.get());
    _pContext = nullptr;
}

void FSR2Integration::CreateContext(const ResolutionInfo &resolution) {
    Assert(_pContext == nullptr);
    _contextDest.maxRenderSize.width = resolution.renderWidth;
    _contextDest.maxRenderSize.height = resolution.renderHeight;
    _contextDest.displaySize.width = resolution.displayWidth;
    _contextDest.displaySize.height = resolution.displayHeight;

    // Enable auto-exposure by default
    _contextDest.flags = FFX_FSR2_ENABLE_AUTO_EXPOSURE;

    if (RenderSetting::Get().GetReversedZ()) {
        _contextDest.flags |= FFX_FSR2_ENABLE_DEPTH_INVERTED | FFX_FSR2_ENABLE_DEPTH_INFINITE;
    }

    GfxDevice *pGfxDevice = GfxDevice::GetInstance();
    if (dx::IsHDRFormat(pGfxDevice->GetRenderTargetFormat())) {
        _contextDest.flags |= FFX_FSR2_ENABLE_HIGH_DYNAMIC_RANGE;
    }

#if MODE_DEBUG
    _contextDest.flags |= FFX_FSR2_ENABLE_DEBUG_CHECKING;
    _contextDest.fpMessage = &FSR2Integration::FfxMsgCallback;
#endif    // #if defined(_DEBUG)

    _pContext = std::make_unique<FfxFsr2Context>();
    FfxErrorCode errorCode = ffxFsr2ContextCreate(_pContext.get(), &_contextDest);
    Assert(errorCode == FFX_OK);
}
