#pragma once
#include <memory>
#include <unordered_map>

#include "NRI.h"
#include "NRIMacro.h"
#include "Extensions/NRIHelper.h"
#include "Extensions/NRIWrapperD3D12.h"

#include "NRD.h"
#include "NRDIntegration.h"
#include "NRDSettings.h"
#include "Foundation/NonCopyable.h"

namespace dx {
class Texture;
class ComputeContext;
}

class NriInterface;
class NrdIntegration;   

struct ShadowDenoiseDesc {
	dx::ComputeContext *pComputeContext = nullptr;
    dx::Texture *pMotionVectorTex = nullptr;
    dx::Texture *pNormalRoughnessTex = nullptr;
    dx::Texture *pViewZTex = nullptr;
    // Optional
    dx::Texture *pBaseColorMetallic = nullptr;
    dx::Texture *pShadowDataTex = nullptr;
    dx::Texture *pOutShadowMaskTex = nullptr;
};

class Denoiser : private NonCopyable {
public:
    Denoiser();
    ~Denoiser();
    void OnCreate();
    void OnDestroy();
    void NewFrame();
    void SetCommonSetting(const nrd::CommonSettings &settings);
    void ShadowDenoise(const ShadowDenoiseDesc &denoiseDesc);
    void OnResize(size_t width, size_t height);
private:
    auto GetNRITexture(const dx::Texture *pTexture) -> nri::Texture *;
    void ClearNRITextureMap();
    auto GetNRDTexture(const dx::Texture *pTexture) -> NrdIntegrationTexture;
    void CreateNRD(size_t width, size_t height);
private:
    using NRITextureMap = std::unordered_map<const dx::Texture *, nri::Texture *>;
    // clang-format off
    nri::Device                    *_pNriDevice = nullptr;
    std::unique_ptr<NrdIntegration> _pNrd;
    std::unique_ptr<NriInterface>   _pNriInterface;
     NRITextureMap                  _nriTextureMap;
    // clang-format on
};