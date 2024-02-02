#pragma once
#include <FidelityFX/host/ffx_fsr2.h>
#include "D3d12/Context.h"
#include "Foundation/NonCopyable.h"
#include "Renderer/RenderUtils/ResolutionInfo.hpp"

class RenderView;

namespace dx {
class Texture;
}

class FSR2Integration : NonCopyable {
public:
    // Enum representing the FSR 2 quality modes.
    enum class FSR2ScalePreset {
        Quality = 0,         // 1.5f
        Balanced,            // 1.7f
        Performance,         // 2.f
        UltraPerformance,    // 3.f
        Custom,              // 1.f - 3.f range
    };

    // Enum representing the reactivity mask modes.
    enum class FSR2MaskMode {
        Disabled = 0,
        Manual,
        Auto,
    };

    struct FSR2ExecuteDesc {
        // clang-format off
        dx::ComputeContext  *pComputeContext        = nullptr;
        const RenderView    *pRenderView            = nullptr;
        dx::Texture         *pColorTex              = nullptr;        
        dx::Texture         *pDepthTex              = nullptr;        // d32f+
        dx::Texture         *pMotionVectorTex       = nullptr;        // rg16f+
        dx::Texture         *pExposureTex           = nullptr;        // optional
        dx::Texture         *pReactiveMaskTex       = nullptr;        // optional
        dx::Texture         *pCompositionMaskTex    = nullptr;        // optional
        dx::Texture         *pOutputTex             = nullptr;
        // clang-format on
    };
public:
    FSR2Integration();
    void OnCreate();
    void OnDestroy();
    void OnResize(const ResolutionInfo &resolution);
    void Execute(const FSR2ExecuteDesc &desc);
    void SetScalePreset(FSR2ScalePreset preset);
    auto GetScalePreset() const -> FSR2ScalePreset {
        return _scalePreset;
    }
    void SetUpscaleRatio(float upscaleRatio) {
        _upscaleRatio = upscaleRatio;
    }
    auto GetUpscaleRatio() const -> float {
        return _upscaleRatio;
    }
    auto GetMipMapBias() const -> float {
        return _mipBias;
    }
    void SetSharpness(float sharpness) {
        _sharpness = sharpness;
    }
    auto GetSharpness() const -> float {
        return _sharpness;
    }
    void SetUseMask(bool enable) {
        _useMask = enable;
    }
    bool GetUseMask() const {
        return _useMask;
    }
    auto GetResolutionInfo(size_t width, size_t height) const -> ResolutionInfo;
    void GetJitterOffset(float &jitterX, float &jitterY) const;
private:
    static void FfxMsgCallback(FfxMsgType type, const wchar_t *pMsg);
    void DestroyContext();
    void CreateContext(const ResolutionInfo &resolution);
private:
    // clang-format off
    FSR2ScalePreset                 _scalePreset;
    float                           _upscaleRatio;
    float                           _mipBias;
    float                           _sharpness;
    bool                            _useMask;
    bool                            _RCASSharpen;
    FSR2MaskMode                    _maskMode;
    uint32_t                        _jitterIndex;
    uint32_t                        _jitterPhaseCount;
    ResolutionInfo                  _resolutionInfo;
    FfxFsr2ContextDescription       _contextDest;
    std::unique_ptr<FfxFsr2Context> _pContext;
    // clang-format on
};
