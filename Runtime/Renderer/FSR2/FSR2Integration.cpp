#include "FSR2Integration.h"
#include <limits>
#include <cmath>
#include <imgui.h>
#include <FidelityFX/host/backends/dx12/ffx_dx12.h>
#include <magic_enum.hpp>

#include "Applcation/Application.h"
#include "Components/Camera.h"
#include "D3d12/Device.h"
#include "D3d12/Texture.h"
#include "Foundation/GameTimer.h"
#include "Foundation/Logger.h"
#include "Foundation/StringUtil.h"
#include "Renderer/GfxDevice.h"
#include "Renderer/RenderUtils/RenderSetting.h"
#include "Renderer/RenderUtils/RenderView.h"
#include "Renderer/RenderUtils/UserMarker.h"

FSR2Integration::FSR2Integration()
    : _mipBias(0), _useMask(false), _jitterIndex(0), _jitterPhaseCount(0), _resolutionInfo(), _contextDest() {
}

void FSR2Integration::OnCreate() {
    GfxDevice *pGfxDevice = GfxDevice::GetInstance();
    size_t scratchBufferSize = ffxGetScratchMemorySizeDX12(FFX_FSR2_CONTEXT_COUNT);
    void *pScratchBuffer = std::malloc(scratchBufferSize);
    FfxDevice ffxDevice = ffxGetDeviceDX12(pGfxDevice->GetDevice()->GetNativeDevice());
    FfxErrorCode errorCode = ffxGetInterfaceDX12(&_contextDest.backendInterface,
        ffxDevice,
        pScratchBuffer,
        scratchBufferSize,
        FFX_FSR2_CONTEXT_COUNT);
    Assert(errorCode == FFX_OK);
    _onBuildRenderSettingGUI = GlobalCallbacks::Get().OnBuildRenderSettingGUI.Register(this,
        &FSR2Integration::BuildRenderSettingUI);
}

void FSR2Integration::OnDestroy() {
    DestroyContext();
    if (_contextDest.backendInterface.scratchBuffer != nullptr) {
        std::free(_contextDest.backendInterface.scratchBuffer);
        _contextDest.backendInterface.scratchBuffer = nullptr;
    }
    _onBuildRenderSettingGUI.Release();
}

void FSR2Integration::OnResize(const ResolutionInfo &resolution) {
    DestroyContext();
    CreateContext(resolution);
    _jitterPhaseCount = ffxFsr2GetJitterPhaseCount(resolution.renderWidth, resolution.displayWidth);
    _jitterIndex = 0;
    _resolutionInfo = resolution;
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

    Assert(desc.pDepthTex->GetFormat() == DXGI_FORMAT_D32_FLOAT);

    FfxFsr2DispatchDescription dispatchParameters = {};
    dispatchParameters.commandList = ffxGetCommandListDX12(pComputeContext->GetCommandList());

    D3D12_RESOURCE_STATES computeRead = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
                                        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    pComputeContext->Transition(desc.pColorTex->GetResource(), computeRead);
    dispatchParameters.color = ConvertFfxResource(desc.pColorTex, L"FSR2_Input_Color");

    D3D12_RESOURCE_STATES depthTexState = computeRead;
    if (desc.pDepthTex->GetFlags() & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) {
        // if it is a depth stencil texture, the D3D12_RESOURCE_STATE_DEPTH_READ state is required
        depthTexState = computeRead | D3D12_RESOURCE_STATE_DEPTH_READ;
    }
    pComputeContext->Transition(desc.pDepthTex->GetResource(), depthTexState);
    dispatchParameters.depth = ConvertFfxResource(desc.pDepthTex, L"FSR2_InputDepth");

    pComputeContext->Transition(desc.pMotionVectorTex->GetResource(), computeRead);
    dispatchParameters.motionVectors = ConvertFfxResource(desc.pMotionVectorTex, L"FSR2_Input_MotionVectors");

    if (desc.pExposureTex != nullptr) {
        pComputeContext->Transition(desc.pExposureTex->GetResource(), computeRead);
        dispatchParameters.exposure = ConvertFfxResource(desc.pExposureTex, L"FSR2_InputExposure");
    }

    pComputeContext->Transition(desc.pOutputTex->GetResource(), computeRead);
    dispatchParameters.output = ConvertFfxResource(desc.pOutputTex, L"FSR2_Output");

    const FSR2Config &fsr2Config = RenderSetting::Get().GetFsr2Config();
    if (fsr2Config.maskMode != FSR2MaskMode::Disabled && desc.pReactiveMaskTex != nullptr) {
        pComputeContext->Transition(desc.pReactiveMaskTex->GetResource(), computeRead);
        dispatchParameters.reactive = ConvertFfxResource(desc.pReactiveMaskTex, L"FSR2_InputReactiveMap");
    }
    if (_useMask && desc.pCompositionMaskTex != nullptr) {
        pComputeContext->Transition(desc.pCompositionMaskTex->GetResource(), computeRead);
        dispatchParameters.transparencyAndComposition = ConvertFfxResource(desc.pCompositionMaskTex,
            L"FSR2_TransparencyAndCompositionMap");
    }

    float jitterX, jitterY;
    GetJitterOffset(jitterX, jitterY);
    dispatchParameters.jitterOffset.x = jitterX;
    dispatchParameters.jitterOffset.y = jitterY;
    dispatchParameters.motionVectorScale.x = _resolutionInfo.renderWidth;
    dispatchParameters.motionVectorScale.y = _resolutionInfo.renderHeight;
    dispatchParameters.reset = false;
    dispatchParameters.enableSharpening = fsr2Config.sharpness != 0.f;
    dispatchParameters.sharpness = fsr2Config.sharpness;
    dispatchParameters.frameTimeDelta = GameTimer::Get().GetDeltaTimeMS();
    dispatchParameters.preExposure = RenderSetting::Get().GetExposure();
    dispatchParameters.renderSize.width = _resolutionInfo.renderWidth;
    dispatchParameters.renderSize.height = _resolutionInfo.renderHeight;

    const cbuffer::CbPrePass &cbPrePass = desc.pRenderView->GetCBPrePass();

    // Setup camera params as required
    dispatchParameters.cameraFovAngleVertical = cbPrePass.radianFov;
    if (RenderSetting::Get().GetReversedZ()) {
        dispatchParameters.cameraFar = cbPrePass.nearClip;
        dispatchParameters.cameraNear = FLT_MAX;
    } else {
        dispatchParameters.cameraFar = cbPrePass.farClip;
        dispatchParameters.cameraNear = cbPrePass.nearClip;
    }

    pComputeContext->FlushResourceBarriers();
    FfxErrorCode errorCode = ffxFsr2ContextDispatch(_pContext.get(), &dispatchParameters);
    FFX_ASSERT(errorCode == FFX_OK);

    // ffxFsr2ContextDispatch will modify the descriptor heap. here we bind our own descriptor heap
    pComputeContext->BindDescriptorHeaps();
    ++_jitterIndex;
}

static float CalculateMipBias(float upscalerRatio) {
    return std::log2f(1.f / upscalerRatio) - 1.f + std::numeric_limits<float>::epsilon();
}

auto FSR2Integration::GetResolutionInfo(size_t width, size_t height) const -> ResolutionInfo {
    float jitterX, jitterY;
    GetJitterOffset(jitterX, jitterY);
    float upscaleRatio = RenderSetting::Get().GetFsr2Config().upscaleRatio;
    return ResolutionInfo(width, height, upscaleRatio, jitterX, jitterY);
}

void FSR2Integration::GetJitterOffset(float &jitterX, float &jitterY) const {
    ffxFsr2GetJitterOffset(&jitterX, &jitterY, _jitterIndex, _jitterPhaseCount);
}

auto FSR2Integration::CalculateUpscaleRatio(FSR2ScalePreset preset) -> float {
    switch (preset) {
    case FSR2ScalePreset::Balanced:
        return 1.7f;
    case FSR2ScalePreset::Performance:
        return 2.f;
    case FSR2ScalePreset::UltraPerformance:
        return 3.f;
    case FSR2ScalePreset::Custom:
    case FSR2ScalePreset::Quality:
    default:;
        return 1.5f;
    }
}

void FSR2Integration::FfxMsgCallback(FfxMsgType type, const wchar_t *pMsg) {
    std::string msg = nstd::to_string(pMsg);
    if (type == FFX_MESSAGE_TYPE_ERROR) {
        Logger::Error("FSR2_API_DEBUG_ERROR: {}", msg);
    } else if (type == FFX_MESSAGE_TYPE_WARNING) {
        Logger::Warning("FSR2_API_DEBUG_WARNING: {}", msg);
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

void FSR2Integration::BuildRenderSettingUI() {
    if (!ImGui::TreeNode("FSR2")) {
        return;
    }

    GfxDevice *pGfxDevice = GfxDevice::GetInstance();
    RenderSetting &renderSetting = RenderSetting::Get();
    FSR2Config &fsr2Config = renderSetting.GetFsr2Config();

    const char *kScalePresetItem[] = {"Quality", "Balanced", "Performance", "UltraPerformance", "Custom"};
    int scalePreset = static_cast<int>(fsr2Config.scalePreset);
    bool hasChanged = ImGui::Combo("ScalePreset", &scalePreset, kScalePresetItem, std::size(kScalePresetItem));
    fsr2Config.scalePreset = static_cast<FSR2ScalePreset>(scalePreset);

    if (hasChanged && scalePreset != static_cast<int>(FSR2ScalePreset::Custom)) {
        pGfxDevice->GetDevice()->WaitForGPUFlush();
        fsr2Config.upscaleRatio = CalculateUpscaleRatio(static_cast<FSR2ScalePreset>(scalePreset));
        _mipBias = CalculateMipBias(fsr2Config.upscaleRatio );
        Application::GetInstance()->MakeWindowSizeDirty();
    }

    float upscaleRatio = fsr2Config.upscaleRatio;
    if (scalePreset == static_cast<int>(FSR2ScalePreset::Custom)) {
		hasChanged = ImGui::DragFloat("UpscaleRation", &upscaleRatio, 0.1f, 1.f, 3.f);
        if (hasChanged) {
	        fsr2Config.upscaleRatio = upscaleRatio;
	        _mipBias = CalculateMipBias(upscaleRatio);
	        Application::GetInstance()->MakeWindowSizeDirty();
        }
    }

    const char *kMaskModeItem[] = {"Disabled", "Manual"};
    int maskMode = static_cast<int>(fsr2Config.maskMode);
    if (ImGui::Combo("MaskMode", &maskMode, kMaskModeItem, std::size(kMaskModeItem))) {
	    fsr2Config.maskMode = static_cast<FSR2MaskMode>(maskMode);
    }
    ImGui::DragFloat("Sharpness", &fsr2Config.sharpness, 0.01f, 0.f, 1.f);
    ImGui::TreePop();
}
