#include "Application.h"
#include "D3d12/Device.h"
#include "D3d12/UploadHeap.h"
#include "Foundation/Logger.h"
#include "Foundation/Memory/GarbageCollection.h"
#include "InputSystem/InputSystem.h"
#include "InputSystem/Window.h"
#include "Renderer/GfxDevice.h"
#include "Renderer/GUI/GUI.h"
#include "Renderer/RenderUtils/FrameCaptrue.h"
#include "Renderer/Samples/Renderer.h"
#include "SceneObject/SceneManager.h"
#include "ShaderLoader/ShaderManager.h"
#include "Utils/AssetProjectSetting.h"
#include "Utils/GlobalCallbacks.h"

#include "Renderer/RenderUtils/RenderSetting.h"
#include "Renderer/Samples/GLTFSample.h"
#include "Renderer/Samples/SimpleLighting.h"
#include "Renderer/Samples/SoftShadow.h"
#include "Renderer/Samples/TriangleRenderer.h"
#include "Renderer/Samples/SkyDemo.h"

Application::Application(): _shouldResize(false) {
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
        DXGI_FORMAT_D32_FLOAT);

    ShaderManager::GetInstance()->OnCreate();
    GarbageCollection::GetInstance()->OnCreate();

    // the gpu needs to run the command finish before the resource can be safely released
    GarbageCollection::GetInstance()->SetDelayedReleaseFrames(GfxDevice::GetInstance()->GetNumBackBuffer() + 1);
    SceneManager::GetInstance()->OnCreate();
    GlobalCallbacks::Get().OnCreate.Invoke();
    GUI::Get().OnCreate();

    //_pRenderer = std::make_unique<TriangleRenderer>();
    //_pRenderer = std::make_unique<SimpleLighting>();
    //_pRenderer = std::make_unique<GLTFSample>();
    //_pRenderer = std::make_unique<SoftShadow>();
    _pRenderer = std::make_unique<SkyDemo>();

    _pRenderer->OnCreate();

    // first flush upload heap
    GfxDevice::GetInstance()->GetUploadHeap()->FlushAndFinish();
    GfxDevice::GetInstance()->GetASBuilder()->FlushAndFinish();

    // register resize call back
    pInputSystem->pWindow->SetResizeCallback([=](int width, int height) {
	    MakeWindowSizeDirty();
    });

    pInputSystem->pWindow->SetPrepareMessageCallBack([](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	    return GUI::Get().PollEvent(hwnd, msg, wParam, lParam);
    });

    // first resize
    uint32_t width = pInputSystem->pWindow->GetWidth();
    uint32_t height = pInputSystem->pWindow->GetHeight();
    OnResize(width, height);
}

void Application::OnDestroy() {
    _pRenderer->OnDestroy();
    _pRenderer = nullptr;
    GUI::Get().OnDestroy();
    GlobalCallbacks::Get().OnDestroy.Invoke();

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
    GUI::Get().NewFrame();
    ITick::OnPreUpdate(timer);
    InputSystem::GetInstance()->OnPreUpdate(timer);
    MainThread::ExecuteMainThreadJob(MainThread::PreUpdate, timer);
    GlobalCallbacks::Get().OnPreUpdate.Invoke(std::ref(timer));
    _pRenderer->OnPreUpdate(timer);
}

void Application::OnUpdate(GameTimer &timer) {
    ITick::OnUpdate(timer);
    InputSystem::GetInstance()->OnUpdate(timer);
    MainThread::ExecuteMainThreadJob(MainThread::OnUpdate, timer);
    GlobalCallbacks::Get().OnUpdate.Invoke(std::ref(timer));
    RenderSetting::Get().OnUpdate();
    _pRenderer->OnUpdate(timer);
}

void Application::OnPostUpdate(GameTimer &timer) {
    ITick::OnPostUpdate(timer);
    InputSystem::GetInstance()->OnPostUpdate(timer);
    MainThread::ExecuteMainThreadJob(MainThread::PostUpdate, timer);
    GlobalCallbacks::Get().OnPostUpdate.Invoke(std::ref(timer));
    _pRenderer->OnPostUpdate(timer);
}

void Application::OnPreRender(GameTimer &timer) {
    if (_shouldResize) {
	    InputSystem *pInputSystem = InputSystem::GetInstance();
        OnResize(pInputSystem->pWindow->GetWidth(), pInputSystem->pWindow->GetHeight());
    }

    ITick::OnPreRender(timer);
    InputSystem::GetInstance()->OnPreRender(timer);
    MainThread::ExecuteMainThreadJob(MainThread::PreRender, timer);
    GlobalCallbacks::Get().OnPreRender.Invoke(std::ref(timer));
    _pRenderer->OnPreRender(timer);
}

void Application::OnRender(GameTimer &timer) {
    ITick::OnRender(timer);
    InputSystem::GetInstance()->OnRender(timer);
    MainThread::ExecuteMainThreadJob(MainThread::OnRender, timer);
    GlobalCallbacks::Get().OnRender.Invoke(std::ref(timer));
    _pRenderer->OnRender(timer);
}

void Application::OnPostRender(GameTimer &timer) {
    ITick::OnPostRender(timer);
    InputSystem::GetInstance()->OnPostRender(timer);
    MainThread::ExecuteMainThreadJob(MainThread::PostRender, timer);
    GlobalCallbacks::Get().OnPostRender.Invoke(std::ref(timer));
    _pRenderer->OnPostRender(timer);
    GarbageCollection::GetInstance()->OnPostRender(timer);
}

void Application::OnResize(uint32_t width, uint32_t height) {
    _shouldResize = false;
    GfxDevice::GetInstance()->GetDevice()->WaitForGPUFlush();
    GlobalCallbacks::Get().OnResize.Invoke(width, height);
    _pRenderer->OnResize(width, height);
}

void Application::MakeWindowSizeDirty() {
    _shouldResize = true;
}
