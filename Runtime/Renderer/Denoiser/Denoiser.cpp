#include "Denoiser.h"
#include "NRDIntegrationD3D12.h"
#include "D3d12/Context.h"
#include "D3d12/Texture.h"
#include "Foundation/GameTimer.h"
#include "Renderer/GfxDevice.h"
#include "Renderer/RenderUtils/RenderView.h"

#define NRD_ID(x) static_cast<nrd::Identifier>(nrd::Denoiser::x)

void DenoiserCommonSettings::Update(const RenderView &renderView) {
    constexpr size_t kMatrixSize = sizeof(float) * 16;
    const cbuffer::CbPrePass &cbPrePass = renderView.GetCBPrePass();
    uint16_t renderWidth = cbPrePass.renderSize.x;
    uint16_t renderHeight = cbPrePass.renderSize.y;
    std::memcpy(this->viewToClipMatrix, value_ptr(cbPrePass.matProj), kMatrixSize);
    std::memcpy(this->viewToClipMatrixPrev, value_ptr(cbPrePass.matProj), kMatrixSize);
    std::memcpy(this->worldToViewMatrix, value_ptr(cbPrePass.matView), kMatrixSize);
    std::memcpy(this->worldToViewMatrixPrev, value_ptr(cbPrePass.matView), kMatrixSize);
    this->cameraJitter[0] = cbPrePass.cameraJitter.x;
    this->cameraJitter[1] = cbPrePass.cameraJitter.y;
    this->cameraJitterPrev[0] = cbPrePass.cameraJitterPrev.y;
    this->cameraJitterPrev[1] = cbPrePass.cameraJitterPrev.y;
    this->resourceSize[0] = renderWidth;
    this->resourceSize[1] = renderHeight;
    this->resourceSizePrev[0] = renderWidth;
    this->resourceSizePrev[1] = renderHeight;
    this->rectSize[0] = renderWidth;
    this->rectSize[1] = renderHeight;
    this->rectSizePrev[0] = renderWidth;
    this->rectSizePrev[1] = renderHeight;
}

Denoiser::Denoiser() {
    _pNrd = std::make_unique<NrdIntegrationD3D12>("Nrd");
}

Denoiser::~Denoiser() {
}

void Denoiser::OnCreate() {
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

void Denoiser::SetCommonSetting(const DenoiserCommonSettings &settings) {
    _settings = settings;
}

auto Denoiser::GetCommonSetting() const -> DenoiserCommonSettings {
    return _settings;
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
    _pNrd->SetCommonSettings(_settings);
    _pNrd->SetDenoiserSettings(identifier, &denoiseDesc.settings);
    _pNrd->Denoise(&identifier, 1, denoiseDesc.pComputeContext, userPool);
}

void Denoiser::OnResize(const ResolutionInfo &resolution) {
    _pNrd->Resize(resolution.renderWidth, resolution.renderHeight);
}

void Denoiser::SetTexture(nrd::ResourceType slot, dx::Texture *pTexture) {
    _textures[static_cast<size_t>(slot)] = pTexture;
}

