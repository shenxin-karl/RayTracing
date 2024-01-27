#pragma once
#include <memory>
#include <unordered_map>

#include "NRD.h"
#include "NRDSettings.h"
#include "Foundation/NonCopyable.h"

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
    void ShadowDenoise(const ShadowDenoiseDesc &denoiseDesc);
    void OnResize(size_t width, size_t height);
    void SetTexture(nrd::ResourceType slot, dx::Texture *pTexture);
private:
    std::unique_ptr<NrdIntegrationD3D12> _pNrd;
    std::vector<dx::Texture *> _textures;
};