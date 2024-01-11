#pragma once
#include <queue>
#include <Windows.h>
#include "Foundation/ITick.hpp"

class InputSystem;
enum class MouseState : int {
	LPress,
	LRelease,
	RPress,
	RRelease,
	WheelDown,
	WheelUp,
	Move,
	Wheel,
	Invalid,
	MaxCount,
};

struct MouseEvent {
	bool IsLPress() const { return _state == MouseState::LPress; }
	bool IsLRelease() const { return _state == MouseState::LRelease; }
	bool IsRPress() const { return _state == MouseState::RPress; }
	bool IsRRelease() const { return _state == MouseState::RRelease; }
	bool IsWheelDown() const { return _state == MouseState::WheelDown; }
	bool IsWheelUp() const { return _state == MouseState::WheelUp; }
	bool IsMove() const { return _state == MouseState::Move; }
	bool IsWheel() const { return _state == MouseState::Wheel; }
	bool IsInvalid() const { return _state == MouseState::Invalid; }
	explicit operator bool() const;
public:
	int		    x = 0;
	int		    y = 0;
	MouseState	_state = MouseState::Invalid;
	float	    _offset = 0;
};

class GameTimer;
class Mouse : public ITick {
public:
	Mouse(InputSystem *pInputSystem);
	~Mouse() override;
	void HandleMsg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnPostRender(GameTimer &timer) override;
	auto GetEvent() -> MouseEvent;
	bool GetShowCursor() const;
	void SetShowCursor(bool bShow);
	void AdjustCursorPosition();
	auto GetCursorPosition() const -> POINT;
private:
	static constexpr size_t kEventMaxSize = 0xff;
	POINT _windowCenter;
	POINT _virtualCursorPos;
	POINT _lastCursorPos;
	bool _showCursor = true;
	InputSystem *_pInputSystem = nullptr;
	std::queue<MouseEvent> _events;
};
