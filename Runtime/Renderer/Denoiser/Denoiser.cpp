#include "Denoiser.h"
#include "NRDIntegration.hpp"
#include "D3d12/Context.h"
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

NRDTexture::NRDTexture() {
}

NRDTexture::NRDTexture(nri::Texture *pNriTexture, const dx::Texture *pTexture, nri::AccessAndLayout prevState,
	nri::AccessAndLayout nextState) {

    pEngineTexture = pTexture;
    state.texture = pNriTexture;
    state.mipOffset = 0;
    state.mipNum = pTexture->GetMipCount();
    state.arrayOffset = 0;
    state.arraySize = pTexture->GetDepthOrArraySize();
    state.prevState = prevState;
    state.nextState = nextState;
    texture.state = &state;
    texture.format = nri::nriConvertDXGIFormatToNRI(pTexture->GetFormat());
}

NRDTexture::NRDTexture(const NRDTexture &other) : state(other.state), texture(other.texture) {
    texture.state = &state;
}

Denoiser::Denoiser() {
    _textures.resize(static_cast<size_t>(nrd::ResourceType::MAX_NUM) - 2);
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
    _textures.clear();
    _textures.resize(static_cast<size_t>(nrd::ResourceType::MAX_NUM) - 2);
}

void Denoiser::SetTexture(nrd::ResourceType resourceType, NRDTexture nrdTexture) {
    _textures[static_cast<size_t>(resourceType)] = nrdTexture;
}

void Denoiser::SetCommonSetting(const nrd::CommonSettings &settings) {
    _pNrd->SetCommonSettings(settings);
}

void Denoiser::ShadowDenoise(const ShadowDenoiseDesc &denoiseDesc) {
    using nrd::ResourceType;
    Exception::CondThrow(_textures[static_cast<size_t>(ResourceType::IN_MV)], "motionVectorTex Must be valid");
    Exception::CondThrow(_textures[static_cast<size_t>(ResourceType::IN_NORMAL_ROUGHNESS)], "motionVectorTex Must be valid");
    Exception::CondThrow(_textures[static_cast<size_t>(ResourceType::IN_VIEWZ)], "motionVectorTex Must be valid");
    Exception::CondThrow(denoiseDesc.shadowDataTex, "shadowDataTex Must be valid");
    Exception::CondThrow(denoiseDesc.outputShadowMaskTex, "outputShadowMaskTex Must be valid");

    NrdUserPool userPool = {};
    for (size_t i = 0; i < _textures.size(); ++i) {
        if (_textures[i]) {
			NrdIntegration_SetResource(userPool, static_cast<ResourceType>(i), _textures[i]);
        }
    }
    NrdIntegration_SetResource(userPool, ResourceType::IN_SHADOWDATA, denoiseDesc.shadowDataTex);
    NrdIntegration_SetResource(userPool, ResourceType::OUT_SHADOW_TRANSLUCENCY, denoiseDesc.outputShadowMaskTex);

    nri::CommandBufferD3D12Desc commandBufferDesc {
        denoiseDesc.pComputeContext->GetCommandList(),
        denoiseDesc.pComputeContext->GetCommandAllocator(),
    };
    nri::CommandBuffer *pCommandBuffer= nullptr;
    nri::Result  result = _pNriInterface->CreateCommandBufferD3D12(*_pNriDevice, commandBufferDesc, pCommandBuffer);
    Assert(result == nri::Result::SUCCESS);

	nrd::Identifier identifier = NRD_ID(SIGMA_SHADOW);
    _pNrd->SetDenoiserSettings(identifier, &denoiseDesc.settings);
    _pNrd->Denoise(&identifier, 1, *pCommandBuffer, userPool);

    // Restores the descriptor heap modified by nrd
    denoiseDesc.pComputeContext->BindDynamicDescriptorHeap();
    _pNriInterface->DestroyCommandBuffer(*pCommandBuffer);
    pCommandBuffer = nullptr;
}

void Denoiser::OnResize(size_t width, size_t height) {
    if (_pNrd) {
        _pNrd->Destroy();
    }
    ClearNRITextureMap();
    CreateNRD(width, height);
}

auto Denoiser::CreateNRDTexture(const dx::Texture *pTexture, nri::AccessAndLayout prevState,
	nri::AccessAndLayout nextState) -> NRDTexture {
    NRDTexture ret = {
        GetNRITexture(pTexture),
        pTexture,
        prevState,
        nextState,
    };
    return ret;
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
