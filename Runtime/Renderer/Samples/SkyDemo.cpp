#include "SkyDemo.h"

#include "Components/CameraColtroller.h"
#include "Components/Light.h"
#include "Components/Transform.h"
#include "D3d12/Device.h"
#include "D3d12/FrameResource.h"
#include "D3d12/FrameResourceRing.h"
#include "D3d12/SwapChain.h"
#include "Foundation/ColorUtil.hpp"
#include "InputSystem/InputSystem.h"
#include "InputSystem/Keyboard.h"
#include "Object/GameObject.h"
#include "Renderer/GfxDevice.h"
#include "Renderer/GUI/GUI.h"
#include "Renderer/RenderPasses/AtmospherePass.h"
#include "Renderer/RenderPasses/DeferredLightingPass.h"
#include "Renderer/RenderPasses/ForwardPass.h"
#include "Renderer/RenderPasses/GBufferPass.h"
#include "Renderer/RenderPasses/PostProcessPass.h"
#include "Renderer/RenderUtils/FrameCaptrue.h"
#include "Renderer/RenderUtils/RenderSetting.h"
#include "SceneObject/GLTFLoader.h"
#include "SceneObject/Scene.h"
#include "SceneObject/SceneLightManager.h"
#include "SceneObject/SceneManager.h"
#include "SceneObject/SceneRenderObjectManager.h"
#include "Utils/AssetProjectSetting.h"

SkyDemo::SkyDemo() : _resolutionInfo(), _pScene(nullptr), _pCameraGO(nullptr) {
}

SkyDemo::~SkyDemo() {
}

void SkyDemo::OnCreate() {
    Renderer::OnCreate();
    CreateRenderPass();
    LoadScene();
    SetupCamera();
    SetupLight();
}

void SkyDemo::OnDestroy() {
    Renderer::OnDestroy();
    _pRenderTargetTex.Release();
    _renderTargetRTV.Release();
    _renderTargetSRV.Release();
    _pDepthStencilTex.Release();
    _depthStencilDSV.Release();
    _depthStencilSRV.Release();
    _pPostProcessPass->OnDestroy();
    _pAtmospherePass->OnDestroy();
}

void SkyDemo::OnUpdate(GameTimer &timer) {
    Renderer::OnUpdate(timer);
    ShowFPS();
}

void SkyDemo::OnPreRender(GameTimer &timer) {
    Renderer::OnPreRender(timer);

    const std::vector<GameObject *> &directionalLightObjects = _pScene->GetSceneLightManager()
                                                                   ->GetDirectionalLightObjects();

    _renderView.Step0_OnNewFrame();
    _renderView.Step1_UpdateCameraMatrix(_pCameraGO->GetComponent<Camera>());
    _renderView.Step2_UpdateResolutionInfo(_resolutionInfo);
    if (!directionalLightObjects.empty()) {
        _renderView.Step3_UpdateDirectionalLightInfo(directionalLightObjects.front()->GetComponent<DirectionalLight>());
    }
    _renderView.Step4_Finalize();

    SceneRenderObjectManager *pRenderObjectMgr = _pScene->GetRenderObjectManager();
    pRenderObjectMgr->ClassifyRenderObjects(_pCameraGO->GetTransform()->GetWorldPosition());
}

void SkyDemo::OnRender(GameTimer &timer) {
    Renderer::OnRender(timer);

    bool frameCapture = InputSystem::GetInstance()->pKeyboard->IsKeyClicked(VK_F11);
    if (frameCapture) {
        FrameCapture::BeginFrameCapture(_pSwapChain->GetHWND(), _pDevice);
    }

    PrepareFrame();
    RenderFrame();
    GUI::Get().Render();
    _pSwapChain->Present();

    if (frameCapture) {
        FrameCapture::EndFrameCapture(_pSwapChain->GetHWND(), _pDevice);
        FrameCapture::OpenCaptureInUI();
    }
}

void SkyDemo::OnResize(uint32_t width, uint32_t height) {
    Renderer::OnResize(width, height);
    RecreateWindowSizeDependentResources();

    _resolutionInfo = ResolutionInfo(width, height);
    _pForwardPass->OnResize(_resolutionInfo);
    _pPostProcessPass->OnResize(_resolutionInfo);
    _pAtmospherePass->OnResize(_resolutionInfo);
}

void SkyDemo::PrepareFrame() {
    dx::FrameResource &pFrameResource = _pFrameResourceRing->GetCurrentFrameResource();
    std::shared_ptr<dx::GraphicsContext> pGfxCxt = pFrameResource.AllocGraphicsContext();

    const cbuffer::CbPrePass &cbPrePass = _renderView.GetCBPrePass();
    const cbuffer::CbLighting &cbLighting = _renderView.GetCBLighting();

    D3D12_GPU_VIRTUAL_ADDRESS cbPrePassAddress = pGfxCxt->AllocConstantBuffer(cbPrePass);
    D3D12_GPU_VIRTUAL_ADDRESS cbLightingAddress = pGfxCxt->AllocConstantBuffer(cbLighting);

    AtmospherePass::AtmospherePassArgs atmospherePassArgs = {};
    atmospherePassArgs.pComputeCtx = pGfxCxt.get();
    atmospherePassArgs.pRenderView = &_renderView;
    //_pAtmospherePass->GenerateLut(atmospherePassArgs);

    SceneRenderObjectManager *pRenderObjectMgr = _pScene->GetRenderObjectManager();
    ForwardPass::DrawArgs forwardDrawArgs = {};
    forwardDrawArgs.pGfxCtx = pGfxCxt.get();
    forwardDrawArgs.cbPrePassCBuffer = cbPrePassAddress;
    forwardDrawArgs.cbLightBuffer = cbLightingAddress;

    pGfxCxt->Transition(_pRenderTargetTex->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
    pGfxCxt->Transition(_pDepthStencilTex->GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
    pGfxCxt->ClearRenderTargetView(_renderTargetRTV.GetCpuHandle(), Colors::Black);
    pGfxCxt->ClearDepthStencilView(_depthStencilDSV.GetCpuHandle(),
        D3D12_CLEAR_FLAG_DEPTH,
        RenderSetting::Get().GetDepthClearValue(),
        0);

    pGfxCxt->SetRenderTargets(_renderTargetRTV.GetCpuHandle(), _depthStencilDSV.GetCpuHandle());
    pGfxCxt->SetViewport(_renderView.GetRenderSizeViewport());
    pGfxCxt->SetScissor(_renderView.GetRenderSizeScissorRect());

    _pForwardPass->DrawBatch(pRenderObjectMgr->GetOpaqueRenderObjects(), forwardDrawArgs);
    _pForwardPass->DrawBatch(pRenderObjectMgr->GetAlphaTestRenderObjects(), forwardDrawArgs);
    _pForwardPass->DrawBatch(pRenderObjectMgr->GetTransparentRenderObjects(), forwardDrawArgs);

    pGfxCxt->Transition(_pRenderTargetTex->GetResource(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    pFrameResource.ExecuteContexts(pGfxCxt.get());
}

void SkyDemo::RenderFrame() {
    _pFrameResourceRing->OnBeginFrame();
    dx::FrameResource &pFrameResource = _pFrameResourceRing->GetCurrentFrameResource();
    std::shared_ptr<dx::GraphicsContext> pGfxCxt = pFrameResource.AllocGraphicsContext();

    pGfxCxt->Transition(_pSwapChain->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET);
    pGfxCxt->SetRenderTargets(_pSwapChain->GetCurrentBackBufferRTV());
    pGfxCxt->SetViewport(_renderView.GetDisplaySizeViewport());
    pGfxCxt->SetScissor(_renderView.GetDisplaySizeScissorRect());

    PostProcessPassDrawArgs postProcessPassDrawArgs = {};
    postProcessPassDrawArgs.inputSRV = _renderTargetSRV.GetCpuHandle();
    postProcessPassDrawArgs.pGfxCtx = pGfxCxt.get();
    _pPostProcessPass->Draw(postProcessPassDrawArgs);
    pGfxCxt->Transition(_pSwapChain->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT);
    pFrameResource.ExecuteContexts(pGfxCxt.get());
}

void SkyDemo::CreateRenderPass() {
    _pForwardPass = std::make_unique<ForwardPass>();
    _pPostProcessPass = std::make_unique<PostProcessPass>();
    _pAtmospherePass = std::make_unique<AtmospherePass>();

    _pForwardPass->OnCreate();
    _pPostProcessPass->OnCreate();
    _pAtmospherePass->OnCreate();
}

void SkyDemo::LoadScene() {
    _pScene = SceneManager::GetInstance()->CreateScene("Scene");
    RenderSetting::Get().SetToneMapperType(ToneMapperType::eACESFilm);
    RenderSetting::Get().SetAmbientIntensity(1.2f);
    RenderSetting::Get().SetExposure(1.1f);

    GLTFLoader gltfLoader;
    stdfs::path path = AssetProjectSetting::ToAssetPath("Models/Terrain/Terrain.gltf");
    gltfLoader.Load(path);
    _pScene->AddGameObject(gltfLoader.GetRootGameObject());
}

void SkyDemo::SetupCamera() {
    SharedPtr<GameObject> pGameObject = GameObject::Create();
    pGameObject->AddComponent<Camera>();
    pGameObject->AddComponent<CameraController>()->cameraMoveSpeed = 300;

    Camera *pCamera = pGameObject->GetComponent<Camera>();
    pCamera->SetNearClip(1.f);
    pCamera->SetFarClip(10000.f);

    Transform *pTransform = pGameObject->GetTransform();
    pTransform->SetLocalPosition(glm::vec3(-1000, 1000, 0));
    pTransform->LookAt(glm::vec3(-850, 200, 15));
    _pCameraGO = pGameObject.Get();
    _pScene->AddGameObject(pGameObject);
}

void SkyDemo::SetupLight() {
    SharedPtr<GameObject> pGameObject = GameObject::Create();
    DirectionalLight *pDirectionalLight = pGameObject->AddComponent<DirectionalLight>();
    pDirectionalLight->SetColor(glm::vec3(3.f));

    Transform *pTransform = pGameObject->GetTransform();
    pTransform->SetLocalPosition(glm::vec3(-10, 10, 0));
    pTransform->LookAt(glm::vec3(0.f));
    _pScene->AddGameObject(pGameObject);
}

void SkyDemo::RecreateWindowSizeDependentResources() {
    GfxDevice *pGfxDevice = GfxDevice::GetInstance();
    dx::NativeDevice *device = _pDevice->GetNativeDevice();

    // recreate render target
    D3D12_RESOURCE_DESC renderTargetDesc = {};
    renderTargetDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    renderTargetDesc.Alignment = 0;
    renderTargetDesc.Width = _width;
    renderTargetDesc.Height = _height;
    renderTargetDesc.DepthOrArraySize = 1;
    renderTargetDesc.MipLevels = 1;
    renderTargetDesc.Format = pGfxDevice->GetRenderTargetFormat();
    renderTargetDesc.SampleDesc = {1, 0};
    renderTargetDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    renderTargetDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    D3D12_CLEAR_VALUE renderTargetClearValue = {};
    renderTargetClearValue.Format = pGfxDevice->GetRenderTargetFormat();
    renderTargetClearValue.Color[0] = 0.f;
    renderTargetClearValue.Color[1] = 0.f;
    renderTargetClearValue.Color[2] = 0.f;
    renderTargetClearValue.Color[3] = 1.f;
    _pRenderTargetTex = dx::Texture::Create(_pDevice,
        renderTargetDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &renderTargetClearValue);
    _pRenderTargetTex->SetName("RenderTargetTexture");

    if (_renderTargetRTV.IsNull()) {
        _renderTargetRTV = _pDevice->AllocDescriptor<dx::RTV>(1);
    }
    device->CreateRenderTargetView(_pRenderTargetTex->GetResource(), nullptr, _renderTargetRTV.GetCpuHandle());

    if (_renderTargetSRV.IsNull()) {
        _renderTargetSRV = _pDevice->AllocDescriptor<dx::SRV>(1);
    }
    device->CreateShaderResourceView(_pRenderTargetTex->GetResource(), nullptr, _renderTargetSRV.GetCpuHandle());

    // recreate depth stencil
    D3D12_RESOURCE_DESC depthStencilDesc = renderTargetDesc;
    depthStencilDesc.Format = pGfxDevice->GetDepthStencilFormat();
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    D3D12_CLEAR_VALUE depthStencilClearValue = {};
    depthStencilClearValue.Format = pGfxDevice->GetDepthStencilFormat();
    depthStencilClearValue.DepthStencil.Depth = RenderSetting::Get().GetDepthClearValue();
    depthStencilClearValue.DepthStencil.Stencil = 0;
    _pDepthStencilTex = dx::Texture::Create(_pDevice,
        depthStencilDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &depthStencilClearValue);
    _pDepthStencilTex->SetName("DepthStencilTexture");

    if (_depthStencilDSV.IsNull()) {
        _depthStencilDSV = _pDevice->AllocDescriptor<dx::DSV>(1);
    }
    D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDsv = {};
    depthStencilDsv.Format = pGfxDevice->GetDepthStencilFormat();
    depthStencilDsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    depthStencilDsv.Texture2D.MipSlice = 0;
    device->CreateDepthStencilView(_pDepthStencilTex->GetResource(), &depthStencilDsv, _depthStencilDSV.GetCpuHandle());

    if (_depthStencilSRV.IsNull()) {
        _depthStencilSRV = _pDevice->AllocDescriptor<dx::SRV>(1);
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC depthStencilSrv = {};
    depthStencilSrv.Format = dx::GetTypelessDepthTextureSRVFormat(_pDepthStencilTex->GetFormat());
    depthStencilSrv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    depthStencilSrv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    depthStencilSrv.Texture2D.MostDetailedMip = 0;
    depthStencilSrv.Texture2D.MipLevels = 1;
    depthStencilSrv.Texture2D.PlaneSlice = 0;
    depthStencilSrv.Texture2D.ResourceMinLODClamp = 0.f;
    device->CreateShaderResourceView(_pDepthStencilTex->GetResource(),
        &depthStencilSrv,
        _depthStencilSRV.GetCpuHandle());
}
