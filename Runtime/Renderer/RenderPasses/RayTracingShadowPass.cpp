#include "RayTracingShadowPass.h"
#include "D3d12/Device.h"
#include "D3d12/RootSignature.h"
#include "D3d12/Texture.h"
#include "Renderer/GfxDevice.h"

void RayTracingShadowPass::OnCreate() {
    RenderPass::OnCreate();
    GfxDevice *pGfxDevice = GfxDevice::GetInstance();
    _pShadowMapTex = std::make_unique<dx::Texture>();
    _pGlobalRootSignature = std::make_unique<dx::RootSignature>();
    _pGlobalRootSignature->OnCreate(3);
    _pGlobalRootSignature->At(eScene).InitAsBufferSRV(0);
    _pGlobalRootSignature->At(eRayGenCb).InitAsBufferCBV(0);
    _pGlobalRootSignature->At(eTable0).InitAsDescriptorTable({
        CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1),
        CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0),
    });
    _pGlobalRootSignature->Generate(pGfxDevice->GetDevice());

    _pAlphaTestLocalRootSignature= std::make_unique<dx::RootSignature>();
    _pAlphaTestLocalRootSignature->OnCreate(5, 6);
    _pAlphaTestLocalRootSignature->At(eMaterialIndex).InitAsConstants(1, 0, 1);
    _pAlphaTestLocalRootSignature->At(eVertexBuffer).InitAsBufferSRV(0, 1);
    _pAlphaTestLocalRootSignature->At(eIndexBuffer).InitAsBufferSRV(1, 1);

    CD3DX12_DESCRIPTOR_RANGE1 range = {
        D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
        static_cast<UINT>(-1),
        2,
        1,
        D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE,
    };
    _pAlphaTestLocalRootSignature->At(eAlbedoTextureList).InitAsDescriptorTable({ range });
    _pAlphaTestLocalRootSignature->At(eInstanceMaterial).InitAsBufferSRV(3, 1);
    _pAlphaTestLocalRootSignature->SetStaticSamplers(dx::GetStaticSamplerArray());
    _pAlphaTestLocalRootSignature->Generate(pGfxDevice->GetDevice(), D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
}

void RayTracingShadowPass::OnDestroy() {
    RenderPass::OnDestroy();
    _pShadowMapTex = nullptr;
    _pGlobalRootSignature = nullptr;
    _pAlphaTestLocalRootSignature = nullptr;
    _pRayTracingPSO = nullptr;
}

void RayTracingShadowPass::GenerateShadowMap(const DrawArgs &args) {
}

auto RayTracingShadowPass::GetShadowMapSRV() const -> D3D12_CPU_DESCRIPTOR_HANDLE {
    return _shadowMapSRV.GetCpuHandle();
}

void RayTracingShadowPass::OnResize(size_t width, size_t height) {
    _pShadowMapTex->OnDestroy();

    GfxDevice *pGfxDevice = GfxDevice::GetInstance();
    CD3DX12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16_FLOAT, width, height);
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    _pShadowMapTex->OnCreate(pGfxDevice->GetDevice(), texDesc, D3D12_RESOURCE_STATE_COMMON);

    if (_shadowMapSRV.IsNull()) {
        _shadowMapSRV = pGfxDevice->GetDevice()->AllocDescriptor<dx::SRV>(1);
    }

    dx::NativeDevice *device = pGfxDevice->GetDevice()->GetNativeDevice();
    D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
    srv.Format = _pShadowMapTex->GetFormat();
    srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv.Texture2D.MipLevels = 1;
    srv.Texture2D.MostDetailedMip = 0;
    srv.Texture2D.ResourceMinLODClamp = 0.f;
    srv.Texture2D.PlaneSlice = 0;
    device->CreateShaderResourceView(_pShadowMapTex->GetResource(), &srv, _shadowMapSRV.GetCpuHandle());

    if (_shadowMapUAV.IsNull()) {
        _shadowMapUAV = pGfxDevice->GetDevice()->AllocDescriptor<dx::UAV>(1);
    }

    D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};
    uav.Format = _pShadowMapTex->GetFormat();
    uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uav.Texture2D.MipSlice = 0;
    uav.Texture2D.PlaneSlice = 0;
    device->CreateUnorderedAccessView(_pShadowMapTex->GetResource(), nullptr, &uav, _shadowMapUAV.GetCpuHandle());
}
