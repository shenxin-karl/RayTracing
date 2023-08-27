#pragma once
#include <windows.h>
#include <bitset>
#include <queue>
#include "Foundation/ITick.hpp"

enum class KeyState : int {
    ePressed,
    eReleased,
    eInvalid,
};

struct CharEvent {
    CharEvent() = default;
    CharEvent(KeyState state, unsigned char character) : _state(state), _character(character) {
    }
    auto GetCharacter() const -> unsigned char {
        return _character;
    }
    auto GetState() const -> KeyState {
        return _state;
    }
    auto IsPressed() const -> bool {
        return _state == KeyState::ePressed;
    }
    auto IsInvalid() const -> bool {
        return _state == KeyState::eInvalid;
    }
    operator bool() const {
        return _state != KeyState::eInvalid;
    }
private:
    KeyState _state = KeyState::eInvalid;
    unsigned char _character = 0;
};

struct KeyEvent {
    KeyEvent() = default;
    KeyEvent(KeyState state, unsigned char key) : _state(state), _key(key) {
    }
    auto GetKey() const -> unsigned char {
        return _key;
    }
    auto GetState() const -> KeyState {
        return _state;
    }
    bool IsPressed() const {
        return _state == KeyState::ePressed;
    }
    bool IsReleased() const {
        return _state == KeyState::eReleased;
    }
    bool IsInvalid() const {
        return _state == KeyState::eInvalid;
    }
    operator bool() const {
        return !IsInvalid();
    }
private:
    KeyState _state = KeyState::eInvalid;
    unsigned char _key = 0;    // Use the window's virtual button
};

class GameTimer;
class Keyboard : public ITick {
public:
    static constexpr int kMaxKeyCodeSize = 0xff;
    static constexpr int kMaxQueueSize = 0xff;
    std::bitset<kMaxKeyCodeSize> _keyState;
    std::bitset<kMaxKeyCodeSize> _characterState;
    std::queue<KeyEvent> _keycodeQueue;
    std::queue<CharEvent> _characterQueue;
public:
    Keyboard();

    bool IsKeyPressed(unsigned char key) const {
        return _keyState.test(key);
    }
    bool IsCharPressed(unsigned char key) const {
        return _characterState.test(key);
    }

    /// https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
    auto GetKeyEvent() -> KeyEvent;
    auto GetCharEvent() -> CharEvent;

    template<typename T>
    static void TryDiscardEvent(std::queue<T> &queue);

    void HandleMsg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void OnPostUpdate(GameTimer &timer) override;
};


