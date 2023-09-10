#pragma once
#include <memory>
#include <string>
#include "Foundation/ITick.hpp"
#include "Foundation/Singleton.hpp"

class Mouse;
class Keyboard;
class Window;
class GameTimer;

class InputSystem
    : public ITick
    , public Singleton<InputSystem> {
public:
    InputSystem();
    ~InputSystem() override;
    void OnCreate(const std::string &title, int width, int height);
    void OnDestroy();
    bool ShouldClose() const;
    void OnPreUpdate(GameTimer &timer) override;
    void OnUpdate(GameTimer &timer) override;
    void OnPostUpdate(GameTimer &timer) override;
public:
    std::unique_ptr<Mouse> pMouse = {};
    std::unique_ptr<Keyboard> pKeyboard = {};
    std::unique_ptr<Window> pWindow = {};
};