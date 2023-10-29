#include "Keyboard.h"

Keyboard::Keyboard() {
}

KeyEvent Keyboard::GetKeyEvent() {
    if (_keycodeQueue.empty())
        return KeyEvent{};

    auto res = _keycodeQueue.front();
    _keycodeQueue.pop();
    return res;
}

CharEvent Keyboard::GetCharEvent() {
    if (_characterQueue.empty())
        return CharEvent{};
    auto res = _characterQueue.front();
    _characterQueue.pop();
    return res;
}

void Keyboard::HandleMsg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    bool repeatFilter = false;(lParam & 0x80000000) != 0;     // // filter auto repeat
    switch (msg) {
    case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
        if (!repeatFilter) {
	        _keyState.set(wParam);
	        _keycodeQueue.emplace(KeyState::ePressed, static_cast<char>(wParam));
        }
        break;
    case WM_KEYUP:
	case WM_SYSKEYUP:
        if (!repeatFilter) {
	        _keyState.set(wParam, false);
	        _keycodeQueue.emplace(KeyState::eReleased, static_cast<char>(wParam));
        }
        break;
    case WM_CHAR:
        _characterState.set(wParam);
        _characterQueue.emplace(KeyState::ePressed, static_cast<char>(wParam));
        break;
    default:
        break;
    }
}

void Keyboard::OnPreUpdate(GameTimer &timer) {
}

template<typename T>
void Keyboard::TryDiscardEvent(std::queue<T> &queue) {
    while (queue.size() > Keyboard::kMaxQueueSize) {
        queue.pop();
    }
}

void Keyboard::OnPostRender(GameTimer &timer) {
    TryDiscardEvent(_keycodeQueue);
    TryDiscardEvent(_characterQueue);
	ITick::OnPostRender(timer);
    _preFrameKeyState = _keyState;
    _preFrameCharacterState = _characterState;
}
