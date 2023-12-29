#include "Application.h"
#include "Foundation/Logger.h"
#include "InputSystem/InputSystem.h"
#include "InputSystem/Window.h"
#include "Renderer/Renderer.h"
#include "ShaderLoader/ShaderManager.h"
#include "Utils/AssetProjectSetting.h"
#include "SceneObject/SceneManager.h"
#include "Utils/GlobalCallbacks.h"
#include "Foundation/Memory/GarbageCollection.h"
#include "D3d12/Device.h"
#include "D3d12/UploadHeap.h"
#include "Renderer/RenderUtils/FrameCaptrue.h"
#include "Renderer/GfxDevice.h"

#include "Renderer/TriangleRenderer.h"
#include "Renderer/SimpleLighting.h"
#include "Renderer/GLTFSample.h"
#include "Renderer/SoftShadow.h"


Application::Application() {
}

Application::~Application() {
}

void Application::OnCreate() {
    Logger::OnInstanceCreate();
    AssetProjectSetting::OnInstanceCreate();
    InputSystem::OnInstanceCreate();
    GfxDevice::OnInstanceCreate();
    ShaderManager::OnInstanceCreate();
    GarbageCollection::OnInstanceCreate();
    SceneManager::OnInstanceCreate();

    Logger::GetInstance()->OnCreate();
    AssetProjectSetting::GetInstance()->OnCreate();
    InputSystem *pInputSystem = InputSystem::GetInstance();
    pInputSystem->OnCreate("RayTracing", 1280, 720);
    FrameCapture::Load();
    GfxDevice::GetInstance()->OnCreate(3,
        pInputSystem->pWindow->GetHWND(),
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        DXGI_FORMAT_D24_UNORM_S8_UINT);

    ShaderManager::GetInstance()->OnCreate();
    GarbageCollection::GetInstance()->OnCreate();
    SceneManager::GetInstance()->OnCreate();
    GlobalCallbacks::Get().onCreate.Invoke();

    //_pRenderer = std::make_unique<TriangleRenderer>();
    //_pRenderer = std::make_unique<SimpleLighting>();
    _pRenderer = std::make_unique<SoftShadow>();

    _pRenderer->OnCreate();

    // first flush upload heap
    GfxDevice::GetInstance()->GetUploadHeap()->FlushAndFinish();
    GfxDevice::GetInstance()->GetASBuilder()->FlushAndFinish();

    // register resize call back
    pInputSystem->pWindow->SetResizeCallback([=](int width, int height) { OnResize(width, height); });

    // first resize
    uint32_t width = pInputSystem->pWindow->GetWidth();
    uint32_t height = pInputSystem->pWindow->GetHeight();
    OnResize(width, height);
}

void Application::OnDestroy() {
    _pRenderer->OnDestroy();
    _pRenderer = nullptr;
    GlobalCallbacks::Get().onDestroy.Invoke();

    SceneManager::GetInstance()->OnDestroy();
    SceneManager::GetInstance()->OnDestroy();
    ShaderManager::GetInstance()->OnDestroy();
    GarbageCollection::GetInstance()->OnDestroy();
    GfxDevice::GetInstance()->OnDestroy();
    FrameCapture::Free();
    InputSystem::GetInstance()->OnDestroy();
    AssetProjectSetting::GetInstance()->OnDestroy();
    Logger::GetInstance()->OnDestroy();

    SceneManager::OnInstanceDestroy();
    GarbageCollection::OnInstanceDestroy();
    GfxDevice::OnInstanceDestroy();
    ShaderManager::OnInstanceDestroy();
    AssetProjectSetting::OnInstanceDestroy();
    InputSystem::OnInstanceDestroy();
    Logger::OnInstanceDestroy();
}

bool Application::IsRunning() const {
    return !InputSystem::GetInstance()->ShouldClose();
}

bool Application::IsPaused() const {
    return InputSystem::GetInstance()->IsPaused();
}

void Application::PollEvent(GameTimer &timer) {
    InputSystem::GetInstance()->PollEvent(timer);
}

void Application::OnPreUpdate(GameTimer &timer) {
    ITick::OnPreUpdate(timer);
    InputSystem::GetInstance()->OnPreUpdate(timer);
    MainThread::ExecuteMainThreadJob(MainThread::PreUpdate, timer);
    GlobalCallbacks::Get().onPreUpdate.Invoke(std::ref(timer));
    _pRenderer->OnPreUpdate(timer);
}

void Application::OnUpdate(GameTimer &timer) {
    ITick::OnUpdate(timer);
    InputSystem::GetInstance()->OnUpdate(timer);
    MainThread::ExecuteMainThreadJob(MainThread::OnUpdate, timer);
    GlobalCallbacks::Get().onUpdate.Invoke(std::ref(timer));
    _pRenderer->OnUpdate(timer);
}

void Application::OnPostUpdate(GameTimer &timer) {
    ITick::OnPostUpdate(timer);
    InputSystem::GetInstance()->OnPostUpdate(timer);
    MainThread::ExecuteMainThreadJob(MainThread::PostUpdate, timer);
    GlobalCallbacks::Get().onPostUpdate.Invoke(std::ref(timer));
    _pRenderer->OnPostUpdate(timer);
}

void Application::OnPreRender(GameTimer &timer) {
    ITick::OnPreRender(timer);
    InputSystem::GetInstance()->OnPreRender(timer);
    MainThread::ExecuteMainThreadJob(MainThread::PreRender, timer);
    GlobalCallbacks::Get().onPreRender.Invoke(std::ref(timer));
    _pRenderer->OnPreRender(timer);
}

void Application::OnRender(GameTimer &timer) {
    ITick::OnRender(timer);
    InputSystem::GetInstance()->OnRender(timer);
    MainThread::ExecuteMainThreadJob(MainThread::OnRender, timer);
    GlobalCallbacks::Get().onRender.Invoke(std::ref(timer));
    _pRenderer->OnRender(timer);
}

void Application::OnPostRender(GameTimer &timer) {
    ITick::OnPostRender(timer);
    InputSystem::GetInstance()->OnPostRender(timer);
    MainThread::ExecuteMainThreadJob(MainThread::PostRender, timer);
    GlobalCallbacks::Get().onPostRender.Invoke(std::ref(timer));
    _pRenderer->OnPostRender(timer);
    GarbageCollection::GetInstance()->OnPostRender(timer);
}

void Application::OnResize(uint32_t width, uint32_t height) {
    GlobalCallbacks::Get().onResize.Invoke(width, height);
    GfxDevice::GetInstance()->GetDevice()->WaitForGPUFlush();
    _pRenderer->OnResize(width, height);
}
