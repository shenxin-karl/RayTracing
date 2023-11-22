#include "Application.h"
#include "Foundation/Logger.h"
#include "InputSystem/InputSystem.h"
#include "InputSystem/Window.h"
#include "Modules/Pix/Pix.h"
#include "Render/Renderer.h"
#include "Modules/Renderdoc/RenderDoc.h"
#include "ShaderLoader/ShaderManager.h"
#include "Utils/AssetProjectSetting.h"

//#include "Render/TriangleRenderer.h"
#include "Render/SimpleLighting.h"

Application::Application() {
}

Application::~Application() {
}

void Application::OnCreate() {
    Logger::OnInstanceCreate();
    AssetProjectSetting::OnInstanceCreate();
    InputSystem::OnInstanceCreate();
    ShaderManager::OnInstanceCreate();

    Logger::GetInstance()->OnCreate();
    AssetProjectSetting::GetInstance()->OnCreate();
    InputSystem *pInputSystem = InputSystem::GetInstance();
    pInputSystem->OnCreate("RayTracing", 1280, 720);
    ShaderManager::GetInstance()->OnCreate();
    RenderDoc::Load();
    Pix::Load();

    HWND hwnd = pInputSystem->pWindow->GetHWND();
    //_pRenderer = std::make_unique<TriangleRenderer>();
    _pRenderer = std::make_unique<SimpleLighting>();
    _pRenderer->OnCreate(3, hwnd);

    // register resize call back
    pInputSystem->pWindow->SetResizeCallback([=](int width, int height) { OnResize(width, height); });

    // first resize
    uint32_t width = pInputSystem->pWindow->GetWidth();
    uint32_t height = pInputSystem->pWindow->GetHeight();
    OnResize(width, height);
}

void Application::OnDestroy() {
    _pRenderer->OnDestroy();
    ShaderManager::GetInstance()->OnDestroy();
    InputSystem::GetInstance()->OnDestroy();
    AssetProjectSetting::GetInstance()->OnDestroy();
    Logger::GetInstance()->OnDestroy();
    Pix::Free();
    RenderDoc::Free();
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

bool Application::PollEvent(GameTimer &timer) {
    return InputSystem::GetInstance()->PollEvent(timer);
}

void Application::OnPreUpdate(GameTimer &timer) {
    ITick::OnPreUpdate(timer);
    MainThread::ExecuteMainThreadJob(MainThread::PreUpdate, timer);
    InputSystem::GetInstance()->OnPreUpdate(timer);
    _pRenderer->OnPreUpdate(timer);
}

void Application::OnUpdate(GameTimer &timer) {
    ITick::OnUpdate(timer);
    MainThread::ExecuteMainThreadJob(MainThread::OnUpdate, timer);
    InputSystem::GetInstance()->OnUpdate(timer);
    _pRenderer->OnUpdate(timer);
}

void Application::OnPostUpdate(GameTimer &timer) {
    ITick::OnPostUpdate(timer);
    MainThread::ExecuteMainThreadJob(MainThread::PostUpdate, timer);
    InputSystem::GetInstance()->OnPostUpdate(timer);
    _pRenderer->OnPostUpdate(timer);
}

void Application::OnPreRender(GameTimer &timer) {
    ITick::OnPreRender(timer);
    MainThread::ExecuteMainThreadJob(MainThread::PreRender, timer);
    InputSystem::GetInstance()->OnPreRender(timer);
    _pRenderer->OnPreRender(timer);
}

void Application::OnRender(GameTimer &timer) {
    ITick::OnRender(timer);
    MainThread::ExecuteMainThreadJob(MainThread::OnRender, timer);
    InputSystem::GetInstance()->OnRender(timer);
    _pRenderer->OnRender(timer);
}

void Application::OnPostRender(GameTimer &timer) {
    ITick::OnPostRender(timer);
    MainThread::ExecuteMainThreadJob(MainThread::PostRender, timer);
    InputSystem::GetInstance()->OnPostRender(timer);
    _pRenderer->OnPostRender(timer);
}

void Application::OnResize(uint32_t width, uint32_t height) {
    _pRenderer->OnResize(width, height);
}
