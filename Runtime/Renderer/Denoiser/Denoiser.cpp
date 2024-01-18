#include "Denoiser.h"
#include "D3d12/Device.h"
#include "D3d12/ResourceStateTracker.h"
#include "D3d12/Texture.h"
#include "Renderer/GfxDevice.h"
#include "Foundation/CompileEnvInfo.hpp"

class NriInterface
    : public nri::CoreInterface
    , public nri::HelperInterface
    , public nri::WrapperD3D12Interface {};

#define NRD_ID(x) static_cast<nrd::Identifier>(nrd::Denoiser::x)

Denoiser::Denoiser() {
}

Denoiser::~Denoiser() {
}

void Denoiser::OnCreate() {
    GfxDevice *pGfxDevice = GfxDevice::GetInstance();
    nri::DeviceCreationD3D12Desc deviceDesc = {};
    deviceDesc.d3d12Device = pGfxDevice->GetDevice()->GetNativeDevice();
    deviceDesc.d3d12GraphicsQueue = pGfxDevice->GetDevice()->GetGraphicsQueue();
    deviceDesc.enableNRIValidation = CompileEnvInfo::IsModeDebug();

    nri::Result nriResult = nri::nriCreateDeviceFromD3D12Device(deviceDesc, _pNriDevice);
    Assert(nriResult == nri::Result::SUCCESS);

    _pNriInterface = std::make_unique<NriInterface>();
    nriResult = nri::nriGetInterface(*_pNriDevice,
        NRI_INTERFACE(nri::CoreInterface),
        (nri::CoreInterface *)_pNriInterface.get());
    Assert(nriResult == nri::Result::SUCCESS);

    nriResult = nri::nriGetInterface(*_pNriDevice,
        NRI_INTERFACE(nri::HelperInterface),
        (nri::HelperInterface *)_pNriInterface.get());
    Assert(nriResult == nri::Result::SUCCESS);

    // Get appropriate "wrapper" extension (XXX - can be D3D11, D3D12 or VULKAN)
    nriResult = nri::nriGetInterface(*_pNriDevice,
        NRI_INTERFACE(nri::WrapperD3D12Interface),
        (nri::WrapperD3D12Interface *)_pNriInterface.get());
    Assert(nriResult == nri::Result::SUCCESS);
}

void Denoiser::OnDestroy() {
    if (_pNrd != nullptr) {
        _pNrd->Destroy();
        _pNrd = nullptr;
    }
    nri::nriDestroyDevice(*_pNriDevice);
}

void Denoiser::NewFrame() {
    _pNrd->NewFrame();
}

void Denoiser::SetCommonSetting(const nrd::CommonSettings &settings) {
    _pNrd->SetCommonSettings(settings);
}


void Denoiser::ShadowDenoise(const ShadowDenoiseDesc &denoiseDesc) {
	nrd::DenoiserDesc desc = {};
    desc.identifier = NRD_ID(SIGMA_SHADOW);
    desc.denoiser = nrd::Denoiser::SIGMA_SHADOW;
    
	nrd::SigmaSettings settings = {};
    _pNrd->SetDenoiserSettings(NRD_ID(SIGMA_SHADOW), &settings);
    NrdUserPool userPool = {};
    //NrdIntegration_SetResource(userPool, nrd::ResourceType::IN_MV, )
}

void Denoiser::OnResize(size_t width, size_t height) {
    if (_pNrd) {
        _pNrd->Destroy();
    }
    ClearNRITextureMap();
    CreateNRD(width, height);
}

auto Denoiser::GetNRITexture(const dx::Texture *pTexture) -> nri::Texture * {
    auto iter = _nriTextureMap.find(pTexture);
    if (iter != _nriTextureMap.end()) {
	    return iter->second;
    }

	nri::TextureD3D12Desc textureDesc = { pTexture->GetResource() };
    nri::Texture *pNriTexture = nullptr;
    _pNriInterface->CreateTextureD3D12(*_pNriDevice, textureDesc, pNriTexture);
    _nriTextureMap.emplace_hint(iter, std::make_pair(pTexture, pNriTexture));
    return pNriTexture;
}

void Denoiser::ClearNRITextureMap() {
    for (auto &&[_, pNriTexture] : _nriTextureMap) {
	    _pNriInterface->DestroyTexture(*pNriTexture);
    }
    _nriTextureMap.clear();
}

auto Denoiser::GetNRDTexture(const dx::Texture *pTexture) -> NrdIntegrationTexture {
    NrdIntegrationTexture ret = {};
    nri::Texture *pNriTexture = GetNRITexture(pTexture);
    ret.state->texture = pNriTexture;
    ret.state->mipOffset = 0;
    ret.state->mipNum = pTexture->GetMipCount();
    ret.state->arrayOffset = 0;
    ret.state->arraySize = pTexture->GetDepthOrArraySize();
    auto *pCurrentResourceState = dx::GlobalResourceState::FindResourceState(pTexture->GetResource());
    Assert(pCurrentResourceState->subResourceStateMap.empty());
    ret.state->prevState.acessBits =  pCurrentResourceState->state;

    ret.format =  nri::nriConvertDXGIFormatToNRI(pTexture->GetFormat());
}

void Denoiser::CreateNRD(size_t width, size_t height) {
    GfxDevice *pGfxDevice = GfxDevice::GetInstance();
    _pNrd = std::make_unique<NrdIntegration>(pGfxDevice->GetNumBackBuffer(), true);

    const nrd::DenoiserDesc denoiserDescs[] = {
        {NRD_ID(SIGMA_SHADOW), nrd::Denoiser::SIGMA_SHADOW},
    };

    nrd::InstanceCreationDesc instanceCreationDesc = {};
    instanceCreationDesc.denoisers = denoiserDescs;
    instanceCreationDesc.denoisersNum = std::size(denoiserDescs);

    bool result = _pNrd
                      ->Initialize(width, height, instanceCreationDesc, *_pNriDevice, *_pNriInterface, *_pNriInterface);
    Assert(result);
}
