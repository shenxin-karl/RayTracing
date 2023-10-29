#include "Mouse.h"
#include <iostream>
#include "Window.h"
#include "InputSystem.h"

Mouse::Mouse(InputSystem *pInputSystem) : _pInputSystem(pInputSystem) {
	GetCursorPos(&_lastCursorPos);
	ScreenToClient(_pInputSystem->pWindow->GetHWND(), &_lastCursorPos);
}

void Mouse::HandleMsg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	int x = LOWORD(lParam);
	int y = HIWORD(lParam);
	bool isEvent = true;
	MouseEvent mouseEvent = { x, y, MouseState::Invalid, 0.f };
	switch (msg) {
	case WM_LBUTTONDOWN:
		mouseEvent._state = MouseState::LPress;
		break;
	case WM_LBUTTONUP:
		mouseEvent._state = MouseState::LRelease;
		break;
	case WM_RBUTTONDOWN:
		mouseEvent._state = MouseState::RPress;
		break;
	case WM_RBUTTONUP:
		mouseEvent._state = MouseState::RRelease;
		break;
	case WM_MOUSEMOVE:
		mouseEvent._state = MouseState::Move;
		if (!_showCursor) {
			int dx = x - _lastCursorPos.x;
			int dy = y - _lastCursorPos.y;
			if (x == _windowCenter.x && y == _windowCenter.y)
				isEvent = false;

			mouseEvent.x = (_virtualCursorPos.x += dx);
			mouseEvent.y = (_virtualCursorPos.y += dy);
		}
		_lastCursorPos.x = x;
		_lastCursorPos.y = y;
		break;
	case WM_MBUTTONDOWN:
		mouseEvent._state = MouseState::WheelDown;
		break;
	case WM_MBUTTONUP:
		mouseEvent._state = MouseState::WheelUp;
		break;
	case WM_MOUSEWHEEL:
		mouseEvent._offset = GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
		mouseEvent._state = MouseState::Wheel;
		break;
	default:
		isEvent = false;
		break;
	}
	if (isEvent)
		_events.push(mouseEvent);
}

void Mouse::OnPostRender(GameTimer &timer) {
	while (_events.size() > kEventMaxSize) {
		_events.pop();
	}
}

MouseEvent Mouse::GetEvent() {
	if (_events.empty())
		return {};

	auto res = _events.front();
	_events.pop();
	return res;
}

bool Mouse::GetShowCursor() const {
	return _showCursor;
}

void Mouse::SetShowCursor(bool bShow) {
	if (_showCursor != bShow) {
		_showCursor = bShow;
		ShowCursor(bShow);
	}

	if (!bShow) {
		AdjustCursorPosition();
		_virtualCursorPos = _lastCursorPos;
	} else {
		ClipCursor(nullptr);
	}

	// clear _events
	std::queue<MouseEvent> temp = {};
	std::swap(_events, temp);
}

void Mouse::AdjustCursorPosition() {
	RECT rect;
	GetClientRect(_pInputSystem->pWindow->GetHWND(), &rect);
	_windowCenter.x = (rect.right + rect.left) / 2;
	_windowCenter.y = (rect.bottom + rect.top) / 2;
	_lastCursorPos = _windowCenter;
	POINT physicsCursorPos = _lastCursorPos;
	ClientToScreen(_pInputSystem->pWindow->GetHWND(), &physicsCursorPos);
	SetCursorPos(physicsCursorPos.x, physicsCursorPos.y);

	GetWindowRect(_pInputSystem->pWindow->GetHWND(), &rect);
	ClipCursor(&rect);
}

POINT Mouse::GetCursorPosition() const {
	return _showCursor ? _virtualCursorPos : _lastCursorPos;
}

MouseEvent::operator bool() const {
	return !IsInvalid();
}
