#include "GLTFSample.h"
#include "GfxDevice.h"
#include "Components/Camera.h"
#include "Components/CameraColtroller.h"
#include "Components/Light.h"
#include "Components/Transform.h"
#include "D3d12/ASBuilder.h"
#include "D3d12/Device.h"
#include "D3d12/FrameResource.h"
#include "D3d12/FrameResourceRing.h"
#include "D3d12/UploadHeap.h"
#include "Object/GameObject.h"
#include "SceneObject/GLTFLoader.h"
#include "SceneObject/Scene.h"
#include "SceneObject/SceneManager.h"
#include "SceneObject/SceneRenderObjectManager.h"
#include "Utils/AssetProjectSetting.h"

GLTFSample::GLTFSample() : _cbPrePass{} {
}

GLTFSample::~GLTFSample() {
}

void GLTFSample::OnCreate() {
    Renderer::OnCreate();
    InitScene();
    _pUploadHeap->FlushAndFinish();
    _pASBuilder->FlushAndFinish();
}

void GLTFSample::OnDestroy() {
    Renderer::OnDestroy();
    SceneManager::GetInstance()->RemoveScene(_pScene->GetName());
    _pScene = nullptr;
}

void GLTFSample::OnPreRender(GameTimer &timer) {
    Renderer::OnPreRender(timer);

    _cbPrePass = cbuffer::MakePrePass(_pCameraGO);

    Camera *pCamera = Camera::GetAvailableCameras()[0];
    SceneRenderObjectManager *pRenderObjectMgr = _pScene->GetRenderObjectManager();
    pRenderObjectMgr->ClassifyRenderObjects(pCamera->GetGameObject()->GetTransform()->GetWorldPosition());
}

void GLTFSample::OnRender(GameTimer &timer) {
    Renderer::OnRender(timer);

    _pFrameResourceRing->OnBeginFrame();
    dx::FrameResource &pFrameResource = _pFrameResourceRing->GetCurrentFrameResource();
    std::shared_ptr<dx::GraphicsContext> pGfxCxt = pFrameResource.AllocGraphicsContext();

    D3D12_VIEWPORT viewport = {0, 0, static_cast<float>(_width), static_cast<float>(_height), 0.f, 1.f};
    D3D12_RECT scissor = {0, 0, static_cast<LONG>(_width), static_cast<LONG>(_height)};

    pGfxCxt->Transition(_renderTargetTex.GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
    pGfxCxt->Transition(_depthStencilTex.GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
    pGfxCxt->SetRenderTargets(_renderTextureRTV.GetCpuHandle(), _depthStencilDSV.GetCpuHandle());
    pGfxCxt->ClearRenderTargetView(_renderTextureRTV.GetCpuHandle(), Colors::Black);
    pGfxCxt->ClearDepthStencilView(_depthStencilDSV.GetCpuHandle(),
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
        0.f,
        0);
    pGfxCxt->SetViewport(viewport);
    pGfxCxt->SetScissor(scissor);

    SceneRenderObjectManager *pRenderObjectMgr = _pScene->GetRenderObjectManager();
    std::vector<RenderObject*> opaqueRenderObjects = pRenderObjectMgr->GetOpaqueRenderObjects();


    _pFrameResourceRing->OnEndFrame();
}

void GLTFSample::OnResize(uint32_t width, uint32_t height) {
    Renderer::OnResize(width, height);

    GfxDevice *pGfxDevice = GfxDevice::GetInstance();
    dx::NativeDevice *device = _pDevice->GetNativeDevice();

    // recreate render target
    _renderTargetTex.OnDestroy();
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
    _renderTargetTex.OnCreate(_pDevice, renderTargetDesc, D3D12_RESOURCE_STATE_COMMON, &renderTargetClearValue);
    _renderTargetTex.SetName("RenderTargetTexture");

    if (_renderTextureRTV.IsNull()) {
        _renderTextureRTV = _pDevice->AllocDescriptor<dx::RTV>(1);
    }
    D3D12_RENDER_TARGET_VIEW_DESC rtv = {};
    rtv.Format = pGfxDevice->GetRenderTargetFormat();
    rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtv.Texture2D.MipSlice = 0;
    rtv.Texture2D.PlaneSlice = 0;
    device->CreateRenderTargetView(_renderTargetTex.GetResource(), &rtv, _renderTextureRTV.GetCpuHandle());

    // recreate depth stencil
    _depthStencilTex.OnDestroy();
    D3D12_RESOURCE_DESC depthStencilDesc = renderTargetDesc;
    renderTargetDesc.Format = pGfxDevice->GetDepthStencilFormat();
    renderTargetDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    D3D12_CLEAR_VALUE depthStencilClearValue = {};
    depthStencilClearValue.Format = pGfxDevice->GetDepthStencilFormat();
    depthStencilClearValue.DepthStencil.Depth = 0.f;
    depthStencilClearValue.DepthStencil.Stencil = 0;
    _depthStencilTex.OnCreate(_pDevice, depthStencilDesc, D3D12_RESOURCE_STATE_COMMON, &depthStencilClearValue);
    _depthStencilTex.SetName("DepthStencilTexture");

    if (_depthStencilDSV.IsNull()) {
        _depthStencilDSV = _pDevice->AllocDescriptor<dx::DSV>(1);
    }
    D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
    dsv.Format = pGfxDevice->GetDepthStencilFormat();
    dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv.Texture2D.MipSlice = 0;
    device->CreateDepthStencilView(_depthStencilTex.GetResource(), &dsv, _depthStencilDSV.GetCpuHandle());
}

void GLTFSample::InitScene() {
    _pScene = SceneManager::GetInstance()->CreateScene("Scene");
    SetupCamera();
    SetupLight();
    LoadGLTF();
}

void GLTFSample::SetupCamera() {
    SharedPtr<GameObject> pGameObject = GameObject::Create();
    pGameObject->AddComponent<Camera>();
    pGameObject->AddComponent<CameraController>();
    Transform *pTransform = pGameObject->GetTransform();
    pTransform->SetLocalPosition(glm::vec3(25, 10, 0));
    pTransform->LookAt(glm::vec3(0));
    _pCameraGO = pGameObject.Get();
    _pScene->AddGameObject(pGameObject);
}

void GLTFSample::SetupLight() {
    SharedPtr<GameObject> pGameObject = GameObject::Create();
    pGameObject->AddComponent<DirectionalLight>();

    Transform *pTransform = pGameObject->GetTransform();
    glm::vec3 direction = glm::normalize(glm::vec3(0, -0.7, 0.5));
    glm::vec3 worldPosition(0, 1000, 0);
    glm::quat worldRotation = glm::Direction2Quaternion(direction);
    pTransform->SetWorldTRS(worldPosition, worldRotation, glm::vec3(1.f));
}

void GLTFSample::LoadGLTF() {
    GLTFLoader loader;
    loader.Load(AssetProjectSetting::ToAssetPath("Models/DamagedHelmet/DamagedHelmet.gltf"));
    _pScene->AddGameObject(loader.GetRootGameObject());
}
