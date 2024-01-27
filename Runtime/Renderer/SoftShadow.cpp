#include "SoftShadow.h"
#include "RenderUtils/FrameCaptrue.h"
#include "GfxDevice.h"
#include "RenderSetting.h"
#include "Components/Camera.h"
#include "Components/CameraColtroller.h"
#include "Components/Light.h"
#include "Components/Transform.h"
#include "D3d12/ASBuilder.h"
#include "D3d12/Device.h"
#include "D3d12/FrameResource.h"
#include "D3d12/FrameResourceRing.h"
#include "D3d12/SwapChain.h"
#include "D3d12/UploadHeap.h"
#include "Foundation/Memory/SharedPtr.hpp"
#include "InputSystem/InputSystem.h"
#include "InputSystem/Keyboard.h"
#include "Object/GameObject.h"
#include "RenderPasses/DeferredLightingPass.h"
#include "RenderPasses/GBufferPass.h"
#include "RenderPasses/PostProcessPass.h"
#include "RenderPasses/RayTracingShadowPass.h"
#include "RenderPasses/SkyBoxPass.h"
#include "SceneObject/GLTFLoader.h"
#include "SceneObject/Scene.h"
#include "SceneObject/SceneManager.h"
#include "SceneObject/SceneRayTracingASManager.h"
#include "SceneObject/SceneRenderObjectManager.h"
#include "Utils/AssetProjectSetting.h"
#include "imgui.h"
#include "Denoiser/Denoiser.h"
#include "Foundation/GameTimer.h"
#include "RenderUtils/GUI.h"

SoftShadow::SoftShadow() : _cbPrePass(), _cbLighting(), _pScene(nullptr), _pCameraGO(nullptr) {
}

SoftShadow::~SoftShadow() {
}

void SoftShadow::OnCreate() {
    Renderer::OnCreate();
    CreateRenderPass();
    CreateScene();
    _pUploadHeap->FlushAndFinish();
    _pASBuilder->FlushAndFinish();
}

void SoftShadow::OnDestroy() {
    Renderer::OnDestroy();
    _pGBufferPass->OnDestroy();
    _pPostProcessPass->OnDestroy();
    _pDeferredLightingPass->OnDestroy();
    _pSkyBoxPass->OnDestroy();
    _pRayTracingShadowPass->OnDestroy();
    _pDenoiser->OnDestroy();
    _renderTargetRTV.Release();
    _depthStencilDSV.Release();
    _renderTargetTex.OnDestroy();
    _depthStencilTex.OnDestroy();
}

void SoftShadow::OnUpdate(GameTimer &timer) {
    Renderer::OnUpdate(timer);
}

void SoftShadow::OnPreRender(GameTimer &timer) {
    Renderer::OnPreRender(timer);

    _cbPrePass = cbuffer::MakeCbPrePass(_pCameraGO);
    _cbLighting = cbuffer::MakeCbLighting(_pScene->GetSceneLightManager());

    Camera *pCamera = _pCameraGO->GetComponent<Camera>();
    if (_pPreviousCameraState == nullptr) {
        _pPreviousCameraState = std::make_unique<CameraState>();
        _pCurrentCameraState = std::make_unique<CameraState>();
        _pCurrentCameraState->Update(pCamera);
        *_pPreviousCameraState = *_pCurrentCameraState;
    } else {
        *_pPreviousCameraState = *_pCurrentCameraState;
        _pCurrentCameraState->Update(pCamera);
    }

    SceneRenderObjectManager *pRenderObjectMgr = _pScene->GetRenderObjectManager();
    pRenderObjectMgr->ClassifyRenderObjects(_pCameraGO->GetTransform()->GetWorldPosition());
}

void SoftShadow::OnRender(GameTimer &timer) {
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

void SoftShadow::OnResize(uint32_t width, uint32_t height) {
    Renderer::OnResize(width, height);
    _pGBufferPass->OnResize(width, height);
    _pRayTracingShadowPass->OnResize(width, height);
    _pDenoiser->OnResize(width, height);
    RecreateWindowSizeDependentResources();
}

void SoftShadow::PrepareFrame() {
    dx::FrameResource &pFrameResource = _pFrameResourceRing->GetCurrentFrameResource();
    std::shared_ptr<dx::GraphicsContext> pGfxCxt = pFrameResource.AllocGraphicsContext();

    D3D12_GPU_VIRTUAL_ADDRESS cbPrePass = pGfxCxt->AllocConstantBuffer(_cbPrePass);
    D3D12_GPU_VIRTUAL_ADDRESS cbLighting = pGfxCxt->AllocConstantBuffer(_cbLighting);

    nrd::CommonSettings denoiseCommandSetting = {};
    constexpr size_t kMatrixSize = sizeof(float) * 16;
    std::memcpy(denoiseCommandSetting.viewToClipMatrix, value_ptr(_pCurrentCameraState->matProj), kMatrixSize);
    std::memcpy(denoiseCommandSetting.viewToClipMatrixPrev, value_ptr(_pPreviousCameraState->matProj), kMatrixSize);
    std::memcpy(denoiseCommandSetting.worldToViewMatrix, value_ptr(_pCurrentCameraState->matView), kMatrixSize);
    std::memcpy(denoiseCommandSetting.worldToViewMatrixPrev, value_ptr(_pPreviousCameraState->matView), kMatrixSize);
    denoiseCommandSetting.resourceSize[0] = static_cast<uint16_t>(_width);
    denoiseCommandSetting.resourceSize[1] = static_cast<uint16_t>(_height);
    denoiseCommandSetting.resourceSizePrev[0] = static_cast<uint16_t>(_width);
    denoiseCommandSetting.resourceSizePrev[1] = static_cast<uint16_t>(_height);
    denoiseCommandSetting.rectSize[0] = static_cast<uint16_t>(_width);
    denoiseCommandSetting.rectSize[1] = static_cast<uint16_t>(_height);
    denoiseCommandSetting.rectSizePrev[0] = static_cast<uint16_t>(_width);
    denoiseCommandSetting.rectSizePrev[1] = static_cast<uint16_t>(_height);
    denoiseCommandSetting.accumulationMode = nrd::AccumulationMode::CONTINUE;
    denoiseCommandSetting.frameIndex = GameTimer::Get().GetFrameCount();
    denoiseCommandSetting.isBaseColorMetalnessAvailable = true;
    _pDenoiser->SetCommonSetting(denoiseCommandSetting);

    GBufferPass::DrawArgs gbufferDrawArgs = {};
    gbufferDrawArgs.pGfxCtx = pGfxCxt.get();
    gbufferDrawArgs.cbPrePassCBuffer = cbPrePass;
    gbufferDrawArgs.pDepthBufferResource = _depthStencilTex.GetResource();
    gbufferDrawArgs.depthBufferDSV = _depthStencilDSV.GetCpuHandle();
    _pGBufferPass->PreDraw(gbufferDrawArgs);

    SceneRenderObjectManager *pRenderObjectMgr = _pScene->GetRenderObjectManager();
    _pGBufferPass->DrawBatch(pRenderObjectMgr->GetOpaqueRenderObjects(), gbufferDrawArgs);
    _pGBufferPass->DrawBatch(pRenderObjectMgr->GetAlphaTestRenderObjects(), gbufferDrawArgs);
    _pGBufferPass->PostDraw(gbufferDrawArgs);

    _pDenoiser->SetTexture(nrd::ResourceType::IN_MV, _pGBufferPass->GetGBufferTexture(GBufferPass::eMotionVectorTex));
    _pDenoiser->SetTexture(nrd::ResourceType::IN_NORMAL_ROUGHNESS,
        _pGBufferPass->GetGBufferTexture(GBufferPass::ePackNormalRoughnessTex));
    _pDenoiser->SetTexture(nrd::ResourceType::IN_BASECOLOR_METALNESS,
        _pGBufferPass->GetGBufferTexture(GBufferPass::eAlbedoMetallicTex));
    _pDenoiser->SetTexture(nrd::ResourceType::IN_VIEWZ, _pGBufferPass->GetGBufferTexture(GBufferPass::eViewDepthTex));

    // shadow map
    SceneRayTracingASManager *pSceneRayTracingAsManager = _pScene->GetRayTracingASManager();
    RayTracingShadowPass::DrawArgs shadowPassDrawArgs;
    shadowPassDrawArgs.sceneTopLevelAS = pSceneRayTracingAsManager->GetTopLevelAS()->GetGPUVirtualAddress();
    shadowPassDrawArgs.geometries = pSceneRayTracingAsManager->GetRayTracingGeometries();
    shadowPassDrawArgs.depthTexSRV = _depthStencilSRV.GetCpuHandle();
    shadowPassDrawArgs.lightDirection = _cbLighting.directionalLight.direction;
    shadowPassDrawArgs.zBufferParams = _cbPrePass.zBufferParams;
    shadowPassDrawArgs.matInvViewProj = _cbPrePass.matInvViewProj;
    shadowPassDrawArgs.pComputeContext = pGfxCxt.get();
    shadowPassDrawArgs.pDenoiser = _pDenoiser.get();
    _pRayTracingShadowPass->GenerateShadowMap(shadowPassDrawArgs);

    // deferred lighting pass
    pGfxCxt->Transition(_renderTargetTex.GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    DeferredLightingPass::DrawArgs deferredLightingPassDrawArgs = {};
    deferredLightingPassDrawArgs.width = _width;
    deferredLightingPassDrawArgs.height = _height;
    deferredLightingPassDrawArgs.cbPrePassAddress = cbPrePass;
    deferredLightingPassDrawArgs.cbLightingAddress = cbLighting;
    deferredLightingPassDrawArgs.gBufferSRV[0] = _pGBufferPass->GetGBufferSRV(0);
    deferredLightingPassDrawArgs.gBufferSRV[1] = _pGBufferPass->GetGBufferSRV(1);
    deferredLightingPassDrawArgs.gBufferSRV[2] = _pGBufferPass->GetGBufferSRV(2);
    deferredLightingPassDrawArgs.depthStencilSRV = _depthStencilSRV.GetCpuHandle();
    deferredLightingPassDrawArgs.outputUAV = _renderTargetUAV.GetCpuHandle();
    deferredLightingPassDrawArgs.shadowMaskSRV = _pRayTracingShadowPass->GetShadowMaskSRV();
    deferredLightingPassDrawArgs.pComputeCtx = pGfxCxt.get();
    _pDeferredLightingPass->Draw(deferredLightingPassDrawArgs);

    // skybox pass
    pGfxCxt->Transition(_renderTargetTex.GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
    pGfxCxt->Transition(_depthStencilTex.GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
    pGfxCxt->SetRenderTargets(_renderTargetRTV.GetCpuHandle(), _depthStencilDSV.GetCpuHandle());

    D3D12_VIEWPORT viewport = {0, 0, static_cast<float>(_width), static_cast<float>(_height), 0.f, 1.f};
    D3D12_RECT scissor = {0, 0, static_cast<LONG>(_width), static_cast<LONG>(_height)};
    pGfxCxt->SetViewport(viewport);
    pGfxCxt->SetScissor(scissor);

    SkyBoxPass::DrawArgs skyBoxDrawArgs = {};
    skyBoxDrawArgs.cubeMapSRV = _skyBoxCubeSRV.GetCpuHandle();
    skyBoxDrawArgs.matView = _cbPrePass.matView;
    skyBoxDrawArgs.matProj = _cbPrePass.matProj;
    skyBoxDrawArgs.pGfxCtx = pGfxCxt.get();
    _pSkyBoxPass->Draw(skyBoxDrawArgs);

    pGfxCxt->Transition(_renderTargetTex.GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    pFrameResource.ExecuteContexts(pGfxCxt.get());
}

void SoftShadow::RenderFrame() {
    _pFrameResourceRing->OnBeginFrame();
    dx::FrameResource &pFrameResource = _pFrameResourceRing->GetCurrentFrameResource();
    std::shared_ptr<dx::GraphicsContext> pGfxCxt = pFrameResource.AllocGraphicsContext();
    pGfxCxt->Transition(_pSwapChain->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET);
    pGfxCxt->SetRenderTargets(_pSwapChain->GetCurrentBackBufferRTV());

    D3D12_VIEWPORT viewport = {0, 0, static_cast<float>(_width), static_cast<float>(_height), 0.f, 1.f};
    D3D12_RECT scissor = {0, 0, static_cast<LONG>(_width), static_cast<LONG>(_height)};
    pGfxCxt->SetViewport(viewport);
    pGfxCxt->SetScissor(scissor);

    PostProcessPassDrawArgs postProcessPassDrawArgs = {};
    postProcessPassDrawArgs.width = _width;
    postProcessPassDrawArgs.height = _height;
    postProcessPassDrawArgs.inputSRV = _renderTargetSRV.GetCpuHandle();
    postProcessPassDrawArgs.pGfxCtx = pGfxCxt.get();
    _pPostProcessPass->Draw(postProcessPassDrawArgs);
    pGfxCxt->Transition(_pSwapChain->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT);
    pFrameResource.ExecuteContexts(pGfxCxt.get());
}

void SoftShadow::CreateRenderPass() {
    _pGBufferPass = std::make_unique<GBufferPass>();
    _pPostProcessPass = std::make_unique<PostProcessPass>();
    _pDeferredLightingPass = std::make_unique<DeferredLightingPass>();
    _pSkyBoxPass = std::make_unique<SkyBoxPass>();
    _pRayTracingShadowPass = std::make_unique<RayTracingShadowPass>();
    _pDenoiser = std::make_unique<Denoiser>();

    _pGBufferPass->OnCreate();
    _pPostProcessPass->OnCreate();
    _pDeferredLightingPass->OnCreate();
    _pSkyBoxPass->OnCreate(GfxDevice::GetInstance()->GetRenderTargetFormat());
    _pRayTracingShadowPass->OnCreate();
    _pDenoiser->OnCreate();
}

void SoftShadow::CreateScene() {
    _pScene = SceneManager::GetInstance()->CreateScene("Scene");
    RenderSetting::Get().SetToneMapperType(ToneMapperType::eACESFilm);
    RenderSetting::Get().SetAmbientIntensity(1.2f);
    RenderSetting::Get().SetExposure(1.1f);
    SetupCamera();
    SetupLight();
    LoadGLTF();
    LoadCubeMap();
}

void SoftShadow::SetupCamera() {
    SharedPtr<GameObject> pGameObject = GameObject::Create();
    pGameObject->AddComponent<Camera>();
    pGameObject->AddComponent<CameraController>()->cameraMoveSpeed = 300;

    Camera *pCamera = pGameObject->GetComponent<Camera>();
    pCamera->SetNearClip(1.f);
    pCamera->SetFarClip(10000.f);

    Transform *pTransform = pGameObject->GetTransform();
    pTransform->SetLocalPosition(glm::vec3(-1000, 200, 0));
    pTransform->LookAt(glm::vec3(-850, 200, 15));
    _pCameraGO = pGameObject.Get();
    _pScene->AddGameObject(pGameObject);
}

void SoftShadow::SetupLight() {
    SharedPtr<GameObject> pGameObject = GameObject::Create();
    DirectionalLight *pDirectionalLight = pGameObject->AddComponent<DirectionalLight>();
    pDirectionalLight->SetColor(glm::vec3(3.f));

    Transform *pTransform = pGameObject->GetTransform();
    pTransform->SetLocalPosition(glm::vec3(-10, 10, 0));
    pTransform->LookAt(glm::vec3(0.f));
    _pScene->AddGameObject(pGameObject);
}

void SoftShadow::LoadGLTF() {
    GLTFLoader loader;
    loader.Load(AssetProjectSetting::ToAssetPath("Models/powerplant/powerplant.gltf"));
    SharedPtr<GameObject> pRootGameObject = loader.GetRootGameObject();
    pRootGameObject->GetTransform()->SetLocalScale(glm::vec3(10.f));
    _pScene->AddGameObject(pRootGameObject);
}

void SoftShadow::LoadCubeMap() {
    TextureLoader textureLoader;
    stdfs::path path = AssetProjectSetting::ToAssetPath("Textures/snowcube1024.dds");
    _pSkyBoxCubeMap = textureLoader.LoadFromFile(path, true);
    _skyBoxCubeSRV = textureLoader.GetSRVCube(_pSkyBoxCubeMap.get());
}

void SoftShadow::RecreateWindowSizeDependentResources() {
    GfxDevice *pGfxDevice = GfxDevice::GetInstance();
    dx::NativeDevice *device = _pDevice->GetNativeDevice();

    // recreate render target
    _renderTargetTex.OnDestroy();
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
    _renderTargetTex.OnCreate(_pDevice, renderTargetDesc, D3D12_RESOURCE_STATE_COMMON, &renderTargetClearValue);
    _renderTargetTex.SetName("RenderTargetTexture");

    if (_renderTargetRTV.IsNull()) {
        _renderTargetRTV = _pDevice->AllocDescriptor<dx::RTV>(1);
    }
    D3D12_RENDER_TARGET_VIEW_DESC renderTextureRtv = {};
    renderTextureRtv.Format = pGfxDevice->GetRenderTargetFormat();
    renderTextureRtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    renderTextureRtv.Texture2D.MipSlice = 0;
    renderTextureRtv.Texture2D.PlaneSlice = 0;
    device->CreateRenderTargetView(_renderTargetTex.GetResource(), &renderTextureRtv, _renderTargetRTV.GetCpuHandle());

    if (_renderTargetSRV.IsNull()) {
        _renderTargetSRV = _pDevice->AllocDescriptor<dx::SRV>(1);
    }
    D3D12_SHADER_RESOURCE_VIEW_DESC renderTextureSrv = {};
    renderTextureSrv.Format = pGfxDevice->GetRenderTargetFormat();
    renderTextureSrv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    renderTextureSrv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    renderTextureSrv.Texture2D.MostDetailedMip = 0;
    renderTextureSrv.Texture2D.MipLevels = 1;
    renderTextureSrv.Texture2D.PlaneSlice = 0;
    renderTextureSrv.Texture2D.ResourceMinLODClamp = 0.f;
    device->CreateShaderResourceView(_renderTargetTex.GetResource(),
        &renderTextureSrv,
        _renderTargetSRV.GetCpuHandle());

    if (_renderTargetUAV.IsNull()) {
        _renderTargetUAV = _pDevice->AllocDescriptor<dx::UAV>(1);
    }
    D3D12_UNORDERED_ACCESS_VIEW_DESC renderTextureUav = {};
    renderTextureUav.Format = _renderTargetTex.GetFormat();
    renderTextureUav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    renderTextureUav.Texture2D.MipSlice = 0;
    renderTextureUav.Texture2D.PlaneSlice = 0;
    device->CreateUnorderedAccessView(_renderTargetTex.GetResource(),
        nullptr,
        &renderTextureUav,
        _renderTargetUAV.GetCpuHandle());

    // recreate depth stencil
    _depthStencilTex.OnDestroy();
    D3D12_RESOURCE_DESC depthStencilDesc = renderTargetDesc;
    depthStencilDesc.Format = dx::GetTypelessFormat(pGfxDevice->GetDepthStencilFormat());
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    D3D12_CLEAR_VALUE depthStencilClearValue = {};
    depthStencilClearValue.Format = pGfxDevice->GetDepthStencilFormat();
    depthStencilClearValue.DepthStencil.Depth = RenderSetting::Get().GetDepthClearValue();
    depthStencilClearValue.DepthStencil.Stencil = 0;
    _depthStencilTex.OnCreate(_pDevice, depthStencilDesc, D3D12_RESOURCE_STATE_COMMON, &depthStencilClearValue);
    _depthStencilTex.SetName("DepthStencilTexture");

    if (_depthStencilDSV.IsNull()) {
        _depthStencilDSV = _pDevice->AllocDescriptor<dx::DSV>(1);
    }
    D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDsv = {};
    depthStencilDsv.Format = pGfxDevice->GetDepthStencilFormat();
    depthStencilDsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    depthStencilDsv.Texture2D.MipSlice = 0;
    device->CreateDepthStencilView(_depthStencilTex.GetResource(), &depthStencilDsv, _depthStencilDSV.GetCpuHandle());

    if (_depthStencilSRV.IsNull()) {
        _depthStencilSRV = _pDevice->AllocDescriptor<dx::SRV>(1);
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC depthStencilSrv = {};
    depthStencilSrv.Format = dx::GetTypelessDepthTextureSRVFormat(_depthStencilTex.GetFormat());
    depthStencilSrv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    depthStencilSrv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    depthStencilSrv.Texture2D.MostDetailedMip = 0;
    depthStencilSrv.Texture2D.MipLevels = 1;
    depthStencilSrv.Texture2D.PlaneSlice = 0;
    depthStencilSrv.Texture2D.ResourceMinLODClamp = 0.f;
    device->CreateShaderResourceView(_depthStencilTex.GetResource(), &depthStencilSrv, _depthStencilSRV.GetCpuHandle());
}