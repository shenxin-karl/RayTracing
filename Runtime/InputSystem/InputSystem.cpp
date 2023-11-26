#include "Foundation/GameTimer.h"
#include "InputSystem/InputSystem.h"
#include "InputSystem/Window.h"
#include "InputSystem/Mouse.h"
#include "InputSystem/Keyboard.h"

InputSystem::InputSystem() {
}

InputSystem::~InputSystem() {
}

void InputSystem::OnCreate(const std::string &title, int width, int height) {
    pWindow = std::make_unique<Window>(width, height, title, this);
    pMouse = std::make_unique<Mouse>(this);
    pKeyboard = std::make_unique<Keyboard>();
    pWindow->SetMessageDispatchCallback([=](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        pMouse->HandleMsg(hwnd, msg, wParam, lParam);
        pKeyboard->HandleMsg(hwnd, msg, wParam, lParam);
    });
}

void InputSystem::OnDestroy() {

}

bool InputSystem::IsPaused() {
    return pWindow->IsPaused();
}

bool InputSystem::ShouldClose() const {
    return pWindow->ShouldClose();
}

void InputSystem::PollEvent(GameTimer &timer) {
    pWindow->PollEvent(timer);
}

void InputSystem::OnPreUpdate(GameTimer &timer) {
    pWindow->OnPreUpdate(timer);
    pMouse->OnPreUpdate(timer);
    pKeyboard->OnPreUpdate(timer);
}

void InputSystem::OnUpdate(GameTimer &timer) {
    pWindow->OnUpdate(timer);
    pMouse->OnUpdate(timer);
    pKeyboard->OnUpdate(timer);
}

void InputSystem::OnPostRender(GameTimer &timer) {
	ITick::OnPostRender(timer);
    pWindow->OnPostRender(timer);
    pMouse->OnPostRender(timer);
    pKeyboard->OnPostRender(timer);
}
