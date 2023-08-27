#include <iostream>
#include <format>
#include "GameTimer/GameTimer.h"
#include "InputSystem.h"
#include "Window.h"
#include "Mouse.h"
#include "Keyboard.h"
#include <format>

int main() {
	std::shared_ptr<com::GameTimer> pGameTimer = std::make_unique<com::GameTimer>();
	std::unique_ptr<com::InputSystem> pInputSystem = std::make_unique<com::InputSystem>("Title", 800, 600);
	while (!pInputSystem->shouldClose()) {
		pInputSystem->tick(pGameTimer);
		while (auto charEvent = pInputSystem->pKeyboard->getCharEvent())
			std::cout << charEvent.getCharacter() << std::endl;
		while (auto mouseEvent = pInputSystem->pMouse->getEvent()) {
			switch (mouseEvent._state) {
			case com::MouseState::LPress:
				std::cout << std::format("LPress:({}, {})", mouseEvent.x, mouseEvent.y) << std::endl;
				break;
			case com::MouseState::LRelease:
				std::cout << std::format("LRelease:({}, {})", mouseEvent.x, mouseEvent.y) << std::endl;
				break;
			case com::MouseState::RPress:
				std::cout << std::format("RPress:({}, {})", mouseEvent.x, mouseEvent.y) << std::endl;
				break;
			case com::MouseState::RRelease:
				std::cout << std::format("RRelease:({}, {})", mouseEvent.x, mouseEvent.y) << std::endl;
				break;
			case com::MouseState::Move:
				std::cout << std::format("Move:({}, {})", mouseEvent.x, mouseEvent.y) << std::endl;
				break;
			case com::MouseState::Wheel:
				std::cout << std::format("Wheel:({})", mouseEvent._offset) << std::endl;
				break;
			case com::MouseState::WheelDown:
				std::cout << std::format("WheelDown:({}, {})", mouseEvent.x, mouseEvent.y) << std::endl;
				break;
			case com::MouseState::WheelUp:
				std::cout << std::format("WheelUp:({}, {})", mouseEvent.x, mouseEvent.y) << std::endl;
				break;
			}
		}
	}
	return 0;
}