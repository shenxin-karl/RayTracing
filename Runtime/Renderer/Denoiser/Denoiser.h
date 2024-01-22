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
}    // namespace dx

class NRDTexture {
public:
    NRDTexture();
    NRDTexture(nri::Texture *pNriTexture,
        const dx::Texture *pTexture,
        nri::AccessAndLayout prevState,
        nri::AccessAndLayout nextState);
    NRDTexture(const NRDTexture &other);
    operator NrdIntegrationTexture() const {
	    return texture;
    }
    operator bool() const {
	    return state.texture;
    }
public:
    const dx::Texture *pEngineTexture = nullptr;
    nri::TextureTransitionBarrierDesc state = {};
    NrdIntegrationTexture texture = {};
};

class NriInterface;
class NrdIntegration;

struct ShadowDenoiseDesc {
    dx::ComputeContext *pComputeContext = nullptr;
    NRDTexture shadowDataTex;
    NRDTexture outputShadowMaskTex;
	nrd::SigmaSettings settings = {};
};

class Denoiser : private NonCopyable {
public:
    Denoiser();
    ~Denoiser();
    void OnCreate();
    void OnDestroy();
    void NewFrame();
    void SetTexture(nrd::ResourceType resourceType, NRDTexture nrdTexture);
    void SetCommonSetting(const nrd::CommonSettings &settings);
    void ShadowDenoise(const ShadowDenoiseDesc &denoiseDesc);
    void OnResize(size_t width, size_t height);
    auto CreateNRDTexture(const dx::Texture *pTexture, nri::AccessAndLayout prevState, nri::AccessAndLayout nextState)
        -> NRDTexture;
private:
    auto GetNRITexture(const dx::Texture *pTexture) -> nri::Texture *;
    void ClearNRITextureMap();
    void CreateNRD(size_t width, size_t height);
private:
    using NRITextureMap = std::unordered_map<const dx::Texture *, nri::Texture *>;
    // clang-format off
    nri::Device                    *_pNriDevice = nullptr;
    std::unique_ptr<NrdIntegration> _pNrd;
    std::unique_ptr<NriInterface>   _pNriInterface;
     NRITextureMap                  _nriTextureMap;
    std::vector<NRDTexture>         _textures;
    // clang-format on
};