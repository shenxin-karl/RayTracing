#include "Application.h"
#include "Foundation/Logger.h"
#include "InputSystem/InputSystem.h"
#include "InputSystem/Window.h"
#include "Render/Renderer.h"
#include "Render/TriangleRenderer.h"
#include "Utils/AssetProjectSetting.h"

Application::Application() {
}

Application::~Application() {
}

void Application::OnCreate() {
    Logger::OnInstanceCreate();
    InputSystem::OnInstanceCreate();
    AssetProjectSetting::OnInstanceCreate();

    Logger::GetInstance()->OnCreate();
    AssetProjectSetting::GetInstance()->OnCreate();
    InputSystem *pInputSystem = InputSystem::GetInstance();
    pInputSystem->OnCreate("RayTracing", 1280, 720);

    HWND hwnd = pInputSystem->pWindow->GetHWND();
    _pRenderer = std::make_unique<TriangleRenderer>();
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
    InputSystem::GetInstance()->OnDestroy();
    AssetProjectSetting::GetInstance()->OnDestroy();
    Logger::GetInstance()->OnDestroy();
    AssetProjectSetting::OnInstanceDestroy();
    InputSystem::OnInstanceDestroy();
    Logger::OnInstanceDestroy();
}

bool Application::IsRunning() const {
    return !InputSystem::GetInstance()->ShouldClose();
}

void Application::OnPreUpdate(GameTimer &timer) {
    ITick::OnPreUpdate(timer);
    InputSystem::GetInstance()->OnPreUpdate(timer);
    _pRenderer->OnPreUpdate(timer);
}

void Application::OnUpdate(GameTimer &timer) {
    ITick::OnUpdate(timer);
    InputSystem::GetInstance()->OnUpdate(timer);
    _pRenderer->OnUpdate(timer);
}

void Application::OnPostUpdate(GameTimer &timer) {
    ITick::OnPostUpdate(timer);
    InputSystem::GetInstance()->OnPostUpdate(timer);
    _pRenderer->OnPostUpdate(timer);
}

void Application::OnPreRender(GameTimer &timer) {
    ITick::OnPreRender(timer);
    InputSystem::GetInstance()->OnPreRender(timer);
    _pRenderer->OnPreRender(timer);
}

void Application::OnRender(GameTimer &timer) {
    ITick::OnRender(timer);
    InputSystem::GetInstance()->OnRender(timer);
    _pRenderer->OnRender(timer);
}

void Application::OnPostRender(GameTimer &timer) {
    ITick::OnPostRender(timer);
    InputSystem::GetInstance()->OnPostRender(timer);
    _pRenderer->OnPostRender(timer);
}

void Application::OnResize(uint32_t width, uint32_t height) {
    _pRenderer->OnResize(width, height);
}
