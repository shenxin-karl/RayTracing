#include "Denoiser.h"
#include "NRDIntegration.hpp"
#include "NRDIntegrationD3D12.h"
#include "D3d12/Context.h"
#include "D3d12/Device.h"
#include "D3d12/ResourceStateTracker.h"
#include "D3d12/Texture.h"
#include "Renderer/GfxDevice.h"
#include "Foundation/CompileEnvInfo.hpp"

#define NRD_ID(x) static_cast<nrd::Identifier>(nrd::Denoiser::x)

Denoiser::Denoiser() {
    _pNrd = std::make_unique<NrdIntegrationD3D12>("Nrd");
}

Denoiser::~Denoiser() {
}

void Denoiser::OnCreate() {
    GfxDevice *pGfxDevice = GfxDevice::GetInstance();

    const nrd::DenoiserDesc denoiserDescs[] = {
        {NRD_ID(SIGMA_SHADOW), nrd::Denoiser::SIGMA_SHADOW},
    };

    nrd::InstanceCreationDesc instanceCreationDesc = {};
    instanceCreationDesc.denoisers = denoiserDescs;
    instanceCreationDesc.denoisersNum = std::size(denoiserDescs);
    _pNrd->OnCreate(instanceCreationDesc);

    _textures.resize(static_cast<size_t>(nrd::ResourceType::MAX_NUM) - 2, nullptr);
}

void Denoiser::OnDestroy() {
	_pNrd->OnDestroy();
}

void Denoiser::SetCommonSetting(const nrd::CommonSettings &settings) {
    _pNrd->SetCommonSettings(settings);
}

void Denoiser::ShadowDenoise(const ShadowDenoiseDesc &denoiseDesc) {
    using nrd::ResourceType;
    Exception::CondThrow(_textures[static_cast<size_t>(ResourceType::IN_MV)], "motionVectorTex Must be valid");
    Exception::CondThrow(_textures[static_cast<size_t>(ResourceType::IN_NORMAL_ROUGHNESS)], "motionVectorTex Must be valid");
    Exception::CondThrow(_textures[static_cast<size_t>(ResourceType::IN_VIEWZ)], "motionVectorTex Must be valid");
    Exception::CondThrow(denoiseDesc.pShadowDataTex, "shadowDataTex Must be valid");
    Exception::CondThrow(denoiseDesc.pOutputShadowMaskTex, "outputShadowMaskTex Must be valid");

    NrdUserPoolD3D12 userPool = {};
    for (size_t i = 0; i < _textures.size(); ++i) {
        userPool[i] = _textures[i];
    }

    userPool[static_cast<size_t>(ResourceType::IN_SHADOWDATA)] = denoiseDesc.pShadowDataTex;
    userPool[static_cast<size_t>(ResourceType::OUT_SHADOW_TRANSLUCENCY)] = denoiseDesc.pOutputShadowMaskTex;

	nrd::Identifier identifier = NRD_ID(SIGMA_SHADOW);
    _pNrd->SetDenoiserSettings(identifier, &denoiseDesc.settings);
    _pNrd->Denoise(&identifier, 1, denoiseDesc.pComputeContext, userPool);
}

void Denoiser::OnResize(size_t width, size_t height) {
    _pNrd->Resize(width, height);
}

void Denoiser::SetTexture(nrd::ResourceType slot, dx::Texture *pTexture) {
    _textures[static_cast<size_t>(slot)] = pTexture;
}

