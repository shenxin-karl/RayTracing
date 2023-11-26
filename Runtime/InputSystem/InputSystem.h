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
    bool IsPaused();
    bool ShouldClose() const;
    void PollEvent(GameTimer &timer);
    void OnPreUpdate(GameTimer &timer) override;
    void OnUpdate(GameTimer &timer) override;
    void OnPostRender(GameTimer &timer) override;
public:
    // clang-format off
    std::unique_ptr<Mouse>      pMouse      = nullptr;
    std::unique_ptr<Keyboard>   pKeyboard   = nullptr;
    std::unique_ptr<Window>     pWindow     = nullptr;
    // clang-format on
};
