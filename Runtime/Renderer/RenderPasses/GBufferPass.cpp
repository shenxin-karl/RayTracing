#include "GBufferPass.h"
#include "D3d12/Context.h"
#include "D3d12/D3dUtils.h"
#include "D3d12/Device.h"
#include "Foundation/ColorUtil.hpp"
#include "Renderer/GfxDevice.h"
#include "Renderer/RenderSetting.h"

GBufferPass::GBufferPass() : _width(0), _height(0) {
}

void GBufferPass::OnCreate() {
    GfxDevice *pDevice = GfxDevice::GetInstance();
    _gBufferSRV = pDevice->GetDevice()->AllocDescriptor<dx::SRV>(3);
    _gBufferRTV = pDevice->GetDevice()->AllocDescriptor<dx::RTV>(3);
}

void GBufferPass::OnDestroy() {
    _gBuffer0.OnDestroy();
    _gBuffer1.OnDestroy();
    _gBuffer2.OnDestroy();
    _gBufferRTV.Release();
    _gBufferSRV.Release();
}

void GBufferPass::OnResize(size_t width, size_t height) {
    GfxDevice *pDevice = GfxDevice::GetInstance();

    _gBuffer0.OnDestroy();
    D3D12_RESOURCE_DESC gBuffer0Desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height);
    gBuffer0Desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    _gBuffer0.OnCreate(pDevice->GetDevice(), gBuffer0Desc, D3D12_RESOURCE_STATE_COMMON);

    _gBuffer1.OnDestroy();
    D3D12_RESOURCE_DESC gBuffer1Desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_FLOAT, width, height);
    gBuffer0Desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    _gBuffer1.OnCreate(pDevice->GetDevice(), gBuffer1Desc, D3D12_RESOURCE_STATE_COMMON);

    _gBuffer2.OnDestroy();
    D3D12_RESOURCE_DESC gBuffer2Desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R11G11B10_FLOAT, width, height);
    gBuffer2Desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    _gBuffer2.OnCreate(pDevice->GetDevice(), gBuffer2Desc, D3D12_RESOURCE_STATE_COMMON);

    auto CreateView = [=](dx::Texture &texture, D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CPU_DESCRIPTOR_HANDLE srv) {
        dx::NativeDevice *device = pDevice->GetDevice()->GetNativeDevice();
        D3D12_RENDER_TARGET_VIEW_DESC rtvViewDesc = {};
        rtvViewDesc.Format = texture.GetFormat();
        rtvViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        rtvViewDesc.Texture2D.MipSlice = 0;
        rtvViewDesc.Texture2D.PlaneSlice = 0;
        device->CreateRenderTargetView(texture.GetResource(), &rtvViewDesc, rtv);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvViewDesc = {};
        srvViewDesc.Format = texture.GetFormat();
        srvViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvViewDesc.Texture2D.MostDetailedMip = 0;
        srvViewDesc.Texture2D.MipLevels = 1;
        srvViewDesc.Texture2D.PlaneSlice = 0;
        srvViewDesc.Texture2D.ResourceMinLODClamp = 0.f;
        device->CreateShaderResourceView(texture.GetResource(), &srvViewDesc, srv);
    };

    CreateView(_gBuffer0, _gBufferRTV[0], _gBufferSRV[0]);
    CreateView(_gBuffer1, _gBufferRTV[1], _gBufferSRV[1]);
    CreateView(_gBuffer2, _gBufferRTV[2], _gBufferSRV[2]);
}

auto GBufferPass::GetGBufferSRV(size_t index) const -> D3D12_CPU_DESCRIPTOR_HANDLE {
    Assert(index < 3);
    return _gBufferSRV[index];
}

void GBufferPass::PreDraw(const DrawArgs &args) {
    D3D12_RECT scissor = {0, 0, static_cast<LONG>(_width), static_cast<LONG>(_height)};
    D3D12_VIEWPORT viewport = {
        0.f,
        0.f,
        static_cast<float>(_width),
        static_cast<float>(_height),
        0.f,
        1.f,
    };
    args.pGfxCtx->SetViewport(viewport);
    args.pGfxCtx->SetScissor(scissor);

    auto TranslationAndClearRT = [&](ID3D12Resource *pResource, D3D12_CPU_DESCRIPTOR_HANDLE rtv, glm::vec4 color) {
        args.pGfxCtx->Transition(pResource, D3D12_RESOURCE_STATE_RENDER_TARGET);
        args.pGfxCtx->ClearRenderTargetView(rtv, color);
    };

    TranslationAndClearRT(_gBuffer0.GetResource(), _gBufferRTV[0], Colors::White);
    TranslationAndClearRT(_gBuffer1.GetResource(), _gBufferRTV[1], Colors::White);
    TranslationAndClearRT(_gBuffer2.GetResource(), _gBufferRTV[2], Colors::White);

    args.pGfxCtx->Transition(args.pDepthBufferResource, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    args.pGfxCtx->ClearDepthStencilView(args.depthBufferDSV,
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_DEPTH,
        RenderSetting::Get().GetDepthClearValue(),
        0);
}

void GBufferPass::DrawBatch(const std::vector<RenderObject *> &batchList, const DrawArgs &args) {
    if (batchList.empty()) {
        return;
    }
}

void GBufferPass::PostDraw(const DrawArgs &args) {
}
