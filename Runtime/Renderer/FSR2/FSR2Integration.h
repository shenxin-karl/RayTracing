#pragma once
#include <FidelityFX/host/ffx_fsr2.h>
#include "D3d12/Context.h"
#include "Foundation/NonCopyable.h"
#include "Renderer/RenderUtils/ResolutionInfo.hpp"
#include "Utils/GlobalCallbacks.h"

enum class FSR2ScalePreset;
class RenderView;

namespace dx {
class Texture;
}

class FSR2Integration : NonCopyable {
public:
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
    auto GetMipMapBias() const -> float {
        return _mipBias;
    }
    void SetUseMask(bool enable) {
        _useMask = enable;
    }
    bool GetUseMask() const {
        return _useMask;
    }
    auto GetResolutionInfo(size_t width, size_t height) const -> ResolutionInfo;
    void GetJitterOffset(float &jitterX, float &jitterY) const;
    static auto CalculateUpscaleRatio(FSR2ScalePreset preset) -> float;
private:
    static void FfxMsgCallback(FfxMsgType type, const wchar_t *pMsg);
    void DestroyContext();
    void CreateContext(const ResolutionInfo &resolution);
    void BuildRenderSettingUI();
private:
    // clang-format off
    float                           _mipBias;
    bool                            _useMask;
    uint32_t                        _jitterIndex;
    uint32_t                        _jitterPhaseCount;
    ResolutionInfo                  _resolutionInfo;
    FfxFsr2ContextDescription       _contextDest;
    std::unique_ptr<FfxFsr2Context> _pContext;
    CallbackHandle                  _onBuildRenderSettingGUI;
    // clang-format on
};
