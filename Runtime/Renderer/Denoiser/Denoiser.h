#pragma once
#include <memory>
#include <unordered_map>

#include "NRD.h"
#include "NRDSettings.h"
#include "Foundation/NonCopyable.h"
#include "Renderer/RenderUtils/ResolutionInfo.hpp"

namespace dx {
class Texture;
class ComputeContext;
}    // namespace dx

class NrdIntegrationD3D12;

struct ShadowDenoiseDesc {
    dx::ComputeContext *pComputeContext = nullptr;
    dx::Texture *pShadowDataTex = nullptr;
    dx::Texture *pOutputShadowMaskTex = nullptr;
    nrd::SigmaSettings settings = {};
};

class Denoiser : private NonCopyable {
public:
    Denoiser();
    ~Denoiser();
    void OnCreate();
    void OnDestroy();
    void SetCommonSetting(const nrd::CommonSettings &settings);
    auto GetCommonSetting() const -> nrd::CommonSettings;
    void ShadowDenoise(const ShadowDenoiseDesc &denoiseDesc);
    void OnResize(const ResolutionInfo &resolution);
    void SetTexture(nrd::ResourceType slot, dx::Texture *pTexture);
private:
    // clang-format off
    std::unique_ptr<NrdIntegrationD3D12> _pNrd;
    nrd::CommonSettings                  _settings;
    std::vector<dx::Texture *>           _textures;
    // clang-format off
};