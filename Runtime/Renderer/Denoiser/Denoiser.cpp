#include "Denoiser.h"

#include "NRIMacro.h"
#include "Extensions/NRIHelper.h"
#include "Extensions/NRIWrapperD3D12.h"

#include "NRDIntegration.hpp"
#include "D3d12/Device.h"
#include "Renderer/GfxDevice.h"
#include "Foundation/CompileEnvInfo.hpp"

class NriInterface
    : public nri::CoreInterface
    , public nri::HelperInterface
    , public nri::WrapperD3D12Interface {};

#define NRD_ID(x) static_cast<nrd::Identifier>(nrd::Denoiser::x)

void Denoiser::OnCreate() {
    GfxDevice *pGfxDevice = GfxDevice::GetInstance();
    nri::DeviceCreationD3D12Desc deviceDesc = {};
    deviceDesc.d3d12Device = pGfxDevice->GetDevice()->GetNativeDevice();
    deviceDesc.d3d12GraphicsQueue = pGfxDevice->GetDevice()->GetGraphicsQueue();
    deviceDesc.enableNRIValidation = CompileEnvInfo::IsModeDebug();

    nri::Result nriResult = nri::nriCreateDeviceFromD3D12Device(deviceDesc, _pNriDevice);
    Assert(nriResult == nri::Result::SUCCESS);

    nriResult = nri::nriGetInterface(*_pNriDevice,
        NRI_INTERFACE(nri::CoreInterface),
        (nri::CoreInterface *)_pNrd.get());
    Assert(nriResult == nri::Result::SUCCESS);

    nriResult = nri::nriGetInterface(*_pNriDevice,
        NRI_INTERFACE(nri::HelperInterface),
        (nri::HelperInterface *)_pNrd.get());
    Assert(nriResult == nri::Result::SUCCESS);

    // Get appropriate "wrapper" extension (XXX - can be D3D11, D3D12 or VULKAN)
    nriResult = nri::nriGetInterface(*_pNriDevice,
        NRI_INTERFACE(nri::WrapperD3D12Interface),
        (nri::WrapperD3D12Interface *)_pNrd.get());
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

void Denoiser::OnResize(size_t width, size_t height) {
    if (_pNrd) {
        _pNrd->Destroy();
    }
    CreateNRD(width, height);
}

void Denoiser::CreateNRD(size_t width, size_t height) {
    GfxDevice *pGfxDevice = GfxDevice::GetInstance();
    _pNrd = std::make_unique<NrdIntegration>(pGfxDevice->GetNumBackBuffer(), false);

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
