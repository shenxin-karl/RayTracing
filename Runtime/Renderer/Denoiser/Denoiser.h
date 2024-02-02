#pragma once
#include <memory>
#include <unordered_map>

#include "NRD.h"
#include "NRDSettings.h"
#include "Foundation/NonCopyable.h"
#include "Renderer/RenderUtils/ResolutionInfo.hpp"

class RenderView;

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

struct DenoiserCommonSettings : public nrd::CommonSettings {
public:
    void Update(const RenderView &renderView);
};

class Denoiser : private NonCopyable {
public:
    Denoiser();
    ~Denoiser();
    void OnCreate();
    void OnDestroy();
    void SetCommonSetting(const DenoiserCommonSettings &settings);
    auto GetCommonSetting() const -> DenoiserCommonSettings;
    void ShadowDenoise(const ShadowDenoiseDesc &denoiseDesc);
    void OnResize(const ResolutionInfo &resolution);
    void SetTexture(nrd::ResourceType slot, dx::Texture *pTexture);
private:
    // clang-format off
    std::unique_ptr<NrdIntegrationD3D12> _pNrd;
    DenoiserCommonSettings                  _settings;
    std::vector<dx::Texture *>           _textures;
    // clang-format off
};