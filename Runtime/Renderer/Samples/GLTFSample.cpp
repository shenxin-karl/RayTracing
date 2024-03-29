#include "GLTFSample.h"
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
#include "InputSystem/InputSystem.h"
#include "InputSystem/Keyboard.h"
#include "Object/GameObject.h"
#include "Renderer/GfxDevice.h"
#include "Renderer/GUI/GUI.h"
#include "Renderer/RenderPasses/ForwardPass.h"
#include "Renderer/RenderPasses/PostProcessPass.h"
#include "Renderer/RenderUtils/FrameCaptrue.h"
#include "Renderer/RenderUtils/RenderSetting.h"
#include "SceneObject/GLTFLoader.h"
#include "SceneObject/Scene.h"
#include "SceneObject/SceneLightManager.h"
#include "SceneObject/SceneManager.h"
#include "SceneObject/SceneRenderObjectManager.h"
#include "Utils/AssetProjectSetting.h"

GLTFSample::GLTFSample() {
}

GLTFSample::~GLTFSample() {
}

void GLTFSample::OnCreate() {
    Renderer::OnCreate();
    InitRenderPass();
    InitScene();
    _pUploadHeap->FlushAndFinish();
    _pASBuilder->FlushAndFinish();
}

void GLTFSample::OnDestroy() {
    Renderer::OnDestroy();
    SceneManager::GetInstance()->RemoveScene(_pScene->GetName());
    _pScene = nullptr;

    _pForwardPass->OnDestroy();
    _pPostProcessPass->OnDestroy();
    _pForwardPass = nullptr;
    _pPostProcessPass = nullptr;
}

void GLTFSample::OnUpdate(GameTimer &timer) {
	Renderer::OnUpdate(timer);
    ShowFPS();
}

void GLTFSample::OnPreRender(GameTimer &timer) {
    Renderer::OnPreRender(timer);

    _renderView.Step0_OnNewFrame();
    _renderView.Step1_UpdateCameraMatrix(_pCameraGO->GetComponent<Camera>());
    _renderView.Step2_UpdateResolutionInfo(ResolutionInfo(_width, _height));
	const std::vector<GameObject*> &directionalLightObjects = _pScene->GetSceneLightManager()->GetDirectionalLightObjects();
    if (!directionalLightObjects.empty()) {
		_renderView.Step3_UpdateDirectionalLightInfo(directionalLightObjects.front()->GetComponent<DirectionalLight>());
    }
    _renderView.Step4_Finalize();

    SceneRenderObjectManager *pRenderObjectMgr = _pScene->GetRenderObjectManager();
    pRenderObjectMgr->ClassifyRenderObjects(_pCameraGO->GetTransform()->GetWorldPosition());
}

void GLTFSample::OnRender(GameTimer &timer) {
    Renderer::OnRender(timer);
    bool frameCapture = InputSystem::GetInstance()->pKeyboard->IsKeyClicked(VK_F11);
    if (frameCapture) {
        FrameCapture::BeginFrameCapture(_pSwapChain->GetHWND(), _pDevice);
    }

    PrepareFrame(timer);
	RenderFrame(timer);
    GUI::Get().Render();
	_pSwapChain->Present();

    if (frameCapture) {
	    FrameCapture::EndFrameCapture(_pSwapChain->GetHWND(), _pDevice);
        FrameCapture::OpenCaptureInUI();
    }
}

void GLTFSample::PrepareFrame(GameTimer &timer) {
    dx::FrameResource &pFrameResource = _pFrameResourceRing->GetCurrentFrameResource();
    std::shared_ptr<dx::GraphicsContext> pGfxCxt = pFrameResource.AllocGraphicsContext();

    pGfxCxt->Transition(_renderTargetTex->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
    pGfxCxt->Transition(_depthStencilTex->GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
    pGfxCxt->SetRenderTargets(_renderTextureRTV.GetCpuHandle(), _depthStencilDSV.GetCpuHandle());
    pGfxCxt->ClearRenderTargetView(_renderTextureRTV.GetCpuHandle(), Colors::Black);
    pGfxCxt->ClearDepthStencilView(_depthStencilDSV.GetCpuHandle(),
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
        RenderSetting::Get().GetDepthClearValue(),
        0);

    pGfxCxt->SetViewport(_renderView.GetRenderSizeViewport());
    pGfxCxt->SetScissor(_renderView.GetRenderSizeScissorRect());

    ForwardPass::DrawArgs forwardPassDrawArgs = {};
    forwardPassDrawArgs.pGfxCtx = pGfxCxt.get();
    forwardPassDrawArgs.cbPrePassCBuffer = pGfxCxt->AllocConstantBuffer(_renderView.GetCBPrePass());
    forwardPassDrawArgs.cbLightBuffer = pGfxCxt->AllocConstantBuffer(_renderView.GetCBLighting());

    SceneRenderObjectManager *pRenderObjectMgr = _pScene->GetRenderObjectManager();
    _pForwardPass->DrawBatch(pRenderObjectMgr->GetOpaqueRenderObjects(), forwardPassDrawArgs);
    _pForwardPass->DrawBatch(pRenderObjectMgr->GetAlphaTestRenderObjects(), forwardPassDrawArgs);

    // todo SkyBox pass
    _pForwardPass->DrawBatch(pRenderObjectMgr->GetTransparentRenderObjects(), forwardPassDrawArgs);
    pGfxCxt->Transition(_renderTargetTex->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    pFrameResource.ExecuteContexts(pGfxCxt.get());
}

void GLTFSample::RenderFrame(GameTimer &timer) {
    _pFrameResourceRing->OnBeginFrame();
    dx::FrameResource &pFrameResource = _pFrameResourceRing->GetCurrentFrameResource();
    std::shared_ptr<dx::GraphicsContext> pGfxCxt = pFrameResource.AllocGraphicsContext();
    pGfxCxt->Transition(_pSwapChain->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET);
    pGfxCxt->SetRenderTargets(_pSwapChain->GetCurrentBackBufferRTV());

    pGfxCxt->SetViewport(_renderView.GetDisplaySizeViewport());
    pGfxCxt->SetScissor(_renderView.GetDisplaySizeScissorRect());

    PostProcessPassDrawArgs postProcessPassDrawArgs = {
        _renderTextureSRV.GetCpuHandle(),
        pGfxCxt.get(),
    };
    _pPostProcessPass->Draw(postProcessPassDrawArgs);
    pGfxCxt->Transition(_pSwapChain->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT);
    pFrameResource.ExecuteContexts(pGfxCxt.get());
}

void GLTFSample::OnResize(uint32_t width, uint32_t height) {
    Renderer::OnResize(width, height);

    _pCameraGO->GetComponent<Camera>()->SetAspect(width, height);

    GfxDevice *pGfxDevice = GfxDevice::GetInstance();
    dx::NativeDevice *device = _pDevice->GetNativeDevice();

    // recreate render target
    D3D12_RESOURCE_DESC renderTargetDesc = {};
    renderTargetDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    renderTargetDesc.Alignment = 0;
    renderTargetDesc.Width = width;
    renderTargetDesc.Height = height;
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
    _renderTargetTex = dx::Texture::Create(_pDevice, renderTargetDesc, D3D12_RESOURCE_STATE_COMMON, &renderTargetClearValue);
    _renderTargetTex->SetName("RenderTargetTexture");

    if (_renderTextureRTV.IsNull()) {
        _renderTextureRTV = _pDevice->AllocDescriptor<dx::RTV>(1);
    }
    D3D12_RENDER_TARGET_VIEW_DESC rtv = {};
    rtv.Format = pGfxDevice->GetRenderTargetFormat();
    rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtv.Texture2D.MipSlice = 0;
    rtv.Texture2D.PlaneSlice = 0;
    device->CreateRenderTargetView(_renderTargetTex->GetResource(), &rtv, _renderTextureRTV.GetCpuHandle());

    if (_renderTextureSRV.IsNull()) {
	    _renderTextureSRV = _pDevice->AllocDescriptor<dx::SRV>(1);
    }
    D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
    srv.Format = pGfxDevice->GetRenderTargetFormat();
    srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv.Texture2D.MostDetailedMip = 0;
    srv.Texture2D.MipLevels = 1;
    srv.Texture2D.PlaneSlice = 0;
    srv.Texture2D.ResourceMinLODClamp = 0.f;
    device->CreateShaderResourceView(_renderTargetTex->GetResource(), &srv, _renderTextureSRV.GetCpuHandle());

    // recreate depth stencil
    D3D12_RESOURCE_DESC depthStencilDesc = renderTargetDesc;
    depthStencilDesc.Format = pGfxDevice->GetDepthStencilFormat();
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    D3D12_CLEAR_VALUE depthStencilClearValue = {};
    depthStencilClearValue.Format = pGfxDevice->GetDepthStencilFormat();
    depthStencilClearValue.DepthStencil.Depth = RenderSetting::Get().GetDepthClearValue();
    depthStencilClearValue.DepthStencil.Stencil = 0;
    _depthStencilTex = dx::Texture::Create(_pDevice, depthStencilDesc, D3D12_RESOURCE_STATE_COMMON, &depthStencilClearValue);
    _depthStencilTex->SetName("DepthStencilTexture");

    if (_depthStencilDSV.IsNull()) {
        _depthStencilDSV = _pDevice->AllocDescriptor<dx::DSV>(1);
    }
    D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
    dsv.Format = pGfxDevice->GetDepthStencilFormat();
    dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv.Texture2D.MipSlice = 0;
    device->CreateDepthStencilView(_depthStencilTex->GetResource(), &dsv, _depthStencilDSV.GetCpuHandle());
}

void GLTFSample::InitRenderPass() {
    _pForwardPass = std::make_unique<ForwardPass>();
    _pPostProcessPass = std::make_unique<PostProcessPass>();
    _pForwardPass->OnCreate();
    _pPostProcessPass->OnCreate();
}

void GLTFSample::InitScene() {
    _pScene = SceneManager::GetInstance()->CreateScene("Scene");
    RenderSetting::Get().SetExposure(1.5f);
    SetupCamera();
    SetupLight();
    LoadGLTF();
}

void GLTFSample::SetupCamera() {
    SharedPtr<GameObject> pGameObject = GameObject::Create();
    pGameObject->AddComponent<Camera>();
    pGameObject->AddComponent<CameraController>()->cameraMoveSpeed = 50;
    Transform *pTransform = pGameObject->GetTransform();
    pTransform->SetLocalPosition(glm::vec3(25, 10, -200));
    pTransform->LookAt(glm::vec3(25, 10, 1));
    _pCameraGO = pGameObject.Get();
    _pScene->AddGameObject(pGameObject);
}

void GLTFSample::SetupLight() {
    SharedPtr<GameObject> pGameObject = GameObject::Create();
    DirectionalLight *pDirectionalLight = pGameObject->AddComponent<DirectionalLight>();
    pDirectionalLight->SetColor(glm::vec3(10.f));

    Transform *pTransform = pGameObject->GetTransform();
    pTransform->SetLocalPosition(glm::vec3(-10, 10, 0));
    pTransform->LookAt(glm::vec3(0.f));
    _pScene->AddGameObject(pGameObject);
}

void GLTFSample::LoadGLTF() {
    GLTFLoader loader;
    loader.Load(AssetProjectSetting::ToAssetPath("Models/DamagedHelmet/DamagedHelmet.gltf"));
    SharedPtr<GameObject> pRootGameObject = loader.GetRootGameObject();
    pRootGameObject->GetTransform()->SetLocalScale(glm::vec3(100.f));
    _pScene->AddGameObject(pRootGameObject);
}
