#include "SoftShadow.h"
#include "Renderer.h"
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
#include "Foundation/GameTimer.h"
#include "Foundation/Logger.h"
#include "Foundation/Memory/SharedPtr.hpp"
#include "InputSystem/InputSystem.h"
#include "InputSystem/Keyboard.h"
#include "Object/GameObject.h"
#include "Renderer/GfxDevice.h"
#include "Renderer/Denoiser/Denoiser.h"
#include "Renderer/FSR2/FSR2Integration.h"
#include "Renderer/GUI/GUI.h"
#include "Renderer/RenderPasses/DeferredLightingPass.h"
#include "Renderer/RenderPasses/GBufferPass.h"
#include "Renderer/RenderPasses/PostProcessPass.h"
#include "Renderer/RenderPasses/RayTracingShadowPass.h"
#include "Renderer/RenderPasses/SkyBoxPass.h"
#include "Renderer/RenderUtils/ConstantBufferHelper.h"
#include "Renderer/RenderUtils/FrameCaptrue.h"
#include "Renderer/RenderUtils/RenderSetting.h"
#include "RenderObject/Material.h"
#include "SceneObject/GLTFLoader.h"
#include "SceneObject/Scene.h"
#include "SceneObject/SceneLightManager.h"
#include "SceneObject/SceneManager.h"
#include "SceneObject/SceneRayTracingASManager.h"
#include "SceneObject/SceneRenderObjectManager.h"
#include "Utils/AssetProjectSetting.h"
#include "Utils/BuildInResource.h"

SoftShadow::SoftShadow() : _pScene(nullptr), _pCameraGO(nullptr) {
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
    _pFsr2Pass->OnDestroy();
    _renderTargetRTV.Release();
    _depthStencilDSV.Release();
    _pRenderTargetTex.Release();
    _pDepthStencilTex.Release();
}

void SoftShadow::OnUpdate(GameTimer &timer) {
    Renderer::OnUpdate(timer);
    ShowFPS();
}

void SoftShadow::OnPreRender(GameTimer &timer) {
    Renderer::OnPreRender(timer);

    const std::vector<GameObject *> &directionalLightObjects = _pScene->GetSceneLightManager()
                                                                   ->GetDirectionalLightObjects();
    ResolutionInfo resolutionInfo = _pFsr2Pass->GetResolutionInfo(_width, _height);
    _renderView.Step0_OnNewFrame();
    _renderView.Step1_UpdateCameraMatrix(_pCameraGO->GetComponent<Camera>());
    _renderView.Step2_UpdateResolutionInfo(resolutionInfo);
    if (!directionalLightObjects.empty()) {
        _renderView.Step3_UpdateDirectionalLightInfo(directionalLightObjects.front()->GetComponent<DirectionalLight>());
    }
    _renderView.BeforeFinalizeOptional_SetMipBias(_pFsr2Pass->GetMipMapBias());
    _renderView.Step4_Finalize();

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
    _pCameraGO->GetComponent<Camera>()->SetAspect(width, height);
    ResolutionInfo resolutionInfo = _pFsr2Pass->GetResolutionInfo(width, height);
    _pGBufferPass->OnResize(resolutionInfo);
    _pRayTracingShadowPass->OnResize(resolutionInfo);
    _pDenoiser->OnResize(resolutionInfo);
    _pFsr2Pass->OnResize(resolutionInfo);
    RecreateWindowSizeDependentResources();
}

void SoftShadow::PrepareFrame() {
    dx::FrameResource &pFrameResource = _pFrameResourceRing->GetCurrentFrameResource();
    std::shared_ptr<dx::GraphicsContext> pGfxCxt = pFrameResource.AllocGraphicsContext();

    const cbuffer::CbPrePass &cbPrePass = _renderView.GetCBPrePass();
    const cbuffer::CbLighting &cbLighting = _renderView.GetCBLighting();

    D3D12_GPU_VIRTUAL_ADDRESS cbPrePassAddress = pGfxCxt->AllocConstantBuffer(cbPrePass);
    D3D12_GPU_VIRTUAL_ADDRESS cbLightingAddress = pGfxCxt->AllocConstantBuffer(cbLighting);

	DenoiserCommonSettings denoiseCommandSetting = {};
    denoiseCommandSetting.Update(_renderView);
    denoiseCommandSetting.frameIndex = GameTimer::Get().GetFrameCount();
    denoiseCommandSetting.isBaseColorMetalnessAvailable = true;
    _pDenoiser->SetCommonSetting(denoiseCommandSetting);

    pGfxCxt->SetViewport(_renderView.GetRenderSizeViewport());
    pGfxCxt->SetScissor(_renderView.GetRenderSizeScissorRect());

    GBufferPass::DrawArgs gbufferDrawArgs = {};
    gbufferDrawArgs.pGfxCtx = pGfxCxt.get();
    gbufferDrawArgs.cbPrePassCBuffer = cbPrePassAddress;
    gbufferDrawArgs.pDepthBufferResource = _pDepthStencilTex->GetResource();
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

#if ENABLE_RAY_TRACING
    // 如果加速结构没有构建, 就够就一下, 因为这是静态的场景, 不用每帧都重新构建
    if (_pRegionTopLevelAs == nullptr) {
	    SceneRayTracingASManager *pSceneRayTracingAsManager = _pScene->GetRayTracingASManager();
	    pSceneRayTracingAsManager->BeginBuildBottomLevelAS();
	    _pRegionTopLevelAs = pSceneRayTracingAsManager->BuildMeshBottomLevelAS();
	    pSceneRayTracingAsManager->EndBuildBottomLevelAS();
	    pSceneRayTracingAsManager->BuildTopLevelAS(pGfxCxt.get(), _pRegionTopLevelAs);
    }
#endif

    // shadow map
    RayTracingShadowPass::DrawArgs shadowPassDrawArgs;
    shadowPassDrawArgs.pRegionTopLevelAs = _pRegionTopLevelAs.get();
    shadowPassDrawArgs.depthTexSRV = _depthStencilSRV.GetCpuHandle();
    shadowPassDrawArgs.pRenderView = &_renderView;
    shadowPassDrawArgs.pComputeContext = pGfxCxt.get();
    shadowPassDrawArgs.pDenoiser = _pDenoiser.get();
    _pRayTracingShadowPass->GenerateShadowMap(shadowPassDrawArgs);

    // deferred lighting pass
    pGfxCxt->Transition(_pRenderTargetTex->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    DeferredLightingPass::DispatchArgs deferredLightingPassDrawArgs = {};
    deferredLightingPassDrawArgs.pRenderView = &_renderView;
    deferredLightingPassDrawArgs.cbPrePassAddress = cbPrePassAddress;
    deferredLightingPassDrawArgs.cbLightingAddress = cbLightingAddress;
    deferredLightingPassDrawArgs.gBufferSRV[0] = _pGBufferPass->GetGBufferSRV(0);
    deferredLightingPassDrawArgs.gBufferSRV[1] = _pGBufferPass->GetGBufferSRV(1);
    deferredLightingPassDrawArgs.gBufferSRV[2] = _pGBufferPass->GetGBufferSRV(2);
    deferredLightingPassDrawArgs.depthStencilSRV = _depthStencilSRV.GetCpuHandle();
    deferredLightingPassDrawArgs.outputUAV = _renderTargetUAV.GetCpuHandle();
    deferredLightingPassDrawArgs.shadowMaskSRV = _pRayTracingShadowPass->GetShadowMaskSRV();
    deferredLightingPassDrawArgs.pComputeCtx = pGfxCxt.get();
    _pDeferredLightingPass->Dispatch(deferredLightingPassDrawArgs);

    // skybox pass
    pGfxCxt->Transition(_pRenderTargetTex->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
    pGfxCxt->Transition(_pDepthStencilTex->GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
    pGfxCxt->SetRenderTargets(_renderTargetRTV.GetCpuHandle(), _depthStencilDSV.GetCpuHandle());
    pGfxCxt->SetViewport(_renderView.GetRenderSizeViewport());
    pGfxCxt->SetScissor(_renderView.GetRenderSizeScissorRect());

    SkyBoxPass::DrawArgs skyBoxDrawArgs = {};
    skyBoxDrawArgs.cubeMapSRV = _skyBoxCubeSRV.GetCpuHandle();
    skyBoxDrawArgs.pRenderView = &_renderView;
    skyBoxDrawArgs.pGfxCtx = pGfxCxt.get();
    _pSkyBoxPass->Draw(skyBoxDrawArgs);

    // fsr2
    FSR2Integration::FSR2ExecuteDesc fsr2ExecuteDesc = {};
    fsr2ExecuteDesc.pComputeContext = pGfxCxt.get();
    fsr2ExecuteDesc.pRenderView = &_renderView;
    fsr2ExecuteDesc.pColorTex = _pRenderTargetTex.Get();
    fsr2ExecuteDesc.pDepthTex = _pDepthStencilTex.Get();
    fsr2ExecuteDesc.pMotionVectorTex = _pGBufferPass->GetGBufferTexture(GBufferPass::eMotionVectorTex);
    fsr2ExecuteDesc.pOutputTex = _pRenderTargetTex.Get();
    _pFsr2Pass->Execute(fsr2ExecuteDesc);

    pGfxCxt->Transition(_pRenderTargetTex->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    pFrameResource.ExecuteContexts(pGfxCxt.get());
}

void SoftShadow::RenderFrame() {
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

void SoftShadow::CreateRenderPass() {
    _pGBufferPass = std::make_unique<GBufferPass>();
    _pPostProcessPass = std::make_unique<PostProcessPass>();
    _pDeferredLightingPass = std::make_unique<DeferredLightingPass>();
    _pSkyBoxPass = std::make_unique<SkyBoxPass>();
    _pRayTracingShadowPass = std::make_unique<RayTracingShadowPass>();
    _pDenoiser = std::make_unique<Denoiser>();
    _pFsr2Pass = std::make_unique<FSR2Integration>();

    _pGBufferPass->OnCreate();
    _pPostProcessPass->OnCreate();
    _pDeferredLightingPass->OnCreate();
    _pSkyBoxPass->OnCreate(GfxDevice::GetInstance()->GetRenderTargetFormat());
    _pRayTracingShadowPass->OnCreate();
    _pDenoiser->OnCreate();
    _pFsr2Pass->OnCreate();
}

void SoftShadow::CreateScene() {
    _pScene = SceneManager::GetInstance()->CreateScene("Scene");
    RenderSetting::Get().SetToneMapperType(ToneMapperType::eACESFilm);
    RenderSetting::Get().SetAmbientIntensity(1.2f);
    RenderSetting::Get().SetExposure(1.1f);
    SetupCamera();
    SetupLight();
    CreateCubeObject();
    LoadGLTF();
    LoadSkyBoxTexture();
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

void SoftShadow::CreateCubeObject() {
	SharedPtr<GameObject> pGameObject = GameObject::Create();
    glm::vec3 scale(50.f);
    glm::vec3 translate(-500, 50, -100.f);
    glm::quat rotation = glm::identity<glm::quat>();
    pGameObject->GetTransform()->SetWorldTRS(translate, rotation, scale);
	MeshRenderer *pMeshRenderer = pGameObject->AddComponent<MeshRenderer>();
    std::shared_ptr<Material> pMaterial = std::make_shared<Material>();

    TextureLoader textureLoader;
    stdfs::path path = AssetProjectSetting::ToAssetPath("Textures/WireFence.dds");
	SharedPtr<dx::Texture> pAlbedoTexture = textureLoader.LoadFromFile(path, false);
	dx::SRV albedoSRV = textureLoader.GetSRV2D(pAlbedoTexture.Get());
    pMaterial->SetTexture(Material::eAlbedoTex, pAlbedoTexture, albedoSRV);
    pMaterial->SetRenderGroup(RenderGroup::eAlphaTest);
    pMaterial->SetCutoff(0.8f);

    pMeshRenderer->SetMesh(BuildInResource::Get().GetCubeMesh());
    pMeshRenderer->SetMaterial(pMaterial);
    _pScene->AddGameObject(pGameObject);
}

void SoftShadow::LoadGLTF() {
    GLTFLoader loader;
    loader.Load(AssetProjectSetting::ToAssetPath("Models/powerplant/powerplant.gltf"));
    SharedPtr<GameObject> pRootGameObject = loader.GetRootGameObject();
    pRootGameObject->GetTransform()->SetLocalScale(glm::vec3(10.f));
    _pScene->AddGameObject(pRootGameObject);
}

void SoftShadow::LoadSkyBoxTexture() {
    TextureLoader textureLoader;
    stdfs::path path = AssetProjectSetting::ToAssetPath("Textures/snowcube1024.dds");
    _pSkyBoxCubeMap = textureLoader.LoadFromFile(path, true);
    _skyBoxCubeSRV = textureLoader.GetSRVCube(_pSkyBoxCubeMap.Get());
}

void SoftShadow::RecreateWindowSizeDependentResources() {
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
    _pRenderTargetTex = dx::Texture::Create(_pDevice, renderTargetDesc, D3D12_RESOURCE_STATE_COMMON, &renderTargetClearValue);
    _pRenderTargetTex->SetName("RenderTargetTexture");

    if (_renderTargetRTV.IsNull()) {
        _renderTargetRTV = _pDevice->AllocDescriptor<dx::RTV>(1);
    }
    device->CreateRenderTargetView(_pRenderTargetTex->GetResource(), nullptr, _renderTargetRTV.GetCpuHandle());

    if (_renderTargetSRV.IsNull()) {
        _renderTargetSRV = _pDevice->AllocDescriptor<dx::SRV>(1);
    }
    device->CreateShaderResourceView(_pRenderTargetTex->GetResource(), nullptr, _renderTargetSRV.GetCpuHandle());

    if (_renderTargetUAV.IsNull()) {
        _renderTargetUAV = _pDevice->AllocDescriptor<dx::UAV>(1);
    }
    device->CreateUnorderedAccessView(_pRenderTargetTex->GetResource(),
        nullptr,
        nullptr,
        _renderTargetUAV.GetCpuHandle());

    // recreate depth stencil
    D3D12_RESOURCE_DESC depthStencilDesc = renderTargetDesc;
    depthStencilDesc.Format = pGfxDevice->GetDepthStencilFormat();
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    D3D12_CLEAR_VALUE depthStencilClearValue = {};
    depthStencilClearValue.Format = pGfxDevice->GetDepthStencilFormat();
    depthStencilClearValue.DepthStencil.Depth = RenderSetting::Get().GetDepthClearValue();
    depthStencilClearValue.DepthStencil.Stencil = 0;
    _pDepthStencilTex = dx::Texture::Create(_pDevice, depthStencilDesc, D3D12_RESOURCE_STATE_COMMON, &depthStencilClearValue);
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
    device->CreateShaderResourceView(_pDepthStencilTex->GetResource(), &depthStencilSrv, _depthStencilSRV.GetCpuHandle());
}