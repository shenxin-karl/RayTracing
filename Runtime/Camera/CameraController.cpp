#include "Camera.h"
#include "CameraController.h"
#include "InputSystem/InputSystem.h"
#include "InputSystem/Mouse.h"
#include "InputSystem/Keyboard.h"
#include "Foundation/GameTimer.h"
#include "Foundation/Logger.h"

CameraController::CameraController(std::shared_ptr<Camera> pCamera) : _pCamera(std::move(pCamera)) {
    const CameraData &cameraData = _pCamera->GetCameraData();
    const glm::vec3 &lookAt = cameraData.lookAt;
    const glm::vec3 &lookFrom = cameraData.lookFrom;
    const glm::vec3 &lookUp = cameraData.lookUp;
    glm::vec3 target = normalize(lookAt - lookFrom);
    glm::vec3 upDir = normalize(lookUp);
    _pitch = std::clamp(glm::degrees(std::asin(target.y)), -89.9f, +89.9f);
    _yaw = glm::degrees(std::atan2(target.z, target.x));
    _roll = glm::degrees(std::asin(upDir.y));
}

void CameraController::Update(GameTimer &timer) {
	InputSystem *pInputSystem = InputSystem::GetInstance(); 
    const CameraData &cameraData = _pCamera->GetCameraData();
    while (MouseEvent event = pInputSystem->pMouse->GetEvent()) {
        if (event.IsRPress()) {
            _mouseRightPress = true;
            _lastMousePosition = POINT{event.x, event.y};
        } else if (event.IsRRelease()) {
            _mouseRightPress = false;
        }

        if (!_mouseRightPress)
            continue;

        if (_lastMousePosition.x == -1 && _lastMousePosition.y == -1)
            _lastMousePosition = POINT{event.x, event.y};

        if (event._state == MouseState::Move) {
            float dx = static_cast<float>(event.x - _lastMousePosition.x) * mouseMoveSensitivity;
            float dy = static_cast<float>(event.y - _lastMousePosition.y) * mouseMoveSensitivity;
            SetPitch(_pitch - dy);
            SetYaw(_yaw - dx);
            _lastMousePosition = POINT{event.x, event.y};
        } else if (event._state == MouseState::Wheel) {
            float fovDeviation = event._offset * mouseWheelSensitivity;
            _pCamera->SetFov(std::clamp(cameraData.fov - fovDeviation, 1.f, 89.f));
        }
    }

    while (KeyEvent event = pInputSystem->pKeyboard->GetKeyEvent()) {
        bool isPressed = event.GetState() == KeyState::ePressed;
        switch (event.GetKey()) {
        case 'W':
            _moveState[Forward] = isPressed;
            break;
        case 'S':
            _moveState[backward] = isPressed;
            break;
        case 'A':
            _moveState[Left] = isPressed;
            break;
        case 'D':
            _moveState[Right] = isPressed;
            break;
        case 'Q':
            _moveState[Down] = isPressed;
            break;
        case 'E':
            _moveState[Up] = isPressed;
            break;
        }
    }

    float deltaTime = timer.GetDeltaTime();
    float advance = 0.f;
    float deviation = 0.f;
    float elevationRise = 0.f;
    advance += static_cast<float>(_moveState[Forward]);
    advance -= static_cast<float>(_moveState[backward]);
    deviation += static_cast<float>(_moveState[Right]);
    deviation -= static_cast<float>(_moveState[Left]);
    elevationRise += static_cast<float>(_moveState[Up]);
    elevationRise -= static_cast<float>(_moveState[Down]);

    glm::vec3 lookFrom = cameraData.lookFrom;
    if (advance != 0.f || deviation != 0.f) {
        glm::vec3 targetDir = normalize(cameraData.lookAt - cameraData.lookFrom);
        glm::vec3 lookUpDir = normalize(cameraData.lookUp);
        glm::vec3 forwardDir = normalize(cameraData.lookAt - cameraData.lookFrom);
        glm::vec3 right = cross(lookUpDir, forwardDir);
        glm::vec3 motor = normalize(targetDir * advance + right * deviation) * (deltaTime * cameraMoveSpeed);
        lookFrom += motor;
    }

    if (elevationRise != 0.f) {
        lookFrom.y = lookFrom.y + elevationRise * cameraMoveSpeed * deltaTime;
    }

    _pCamera->SetLookFrom(lookFrom);
    float radianPitch = glm::radians(_pitch);
    float radianYaw = glm::radians(_yaw);
    float sinPitch = std::sin(radianPitch);
    float cosPitch = std::cos(radianPitch);
    float sinYaw = std::sin(radianYaw);
    float cosYaw = std::cos(radianYaw);
    glm::vec3 target = {
        cosPitch * cosYaw,
        sinPitch,
        cosPitch * sinYaw,
    };

    float radianRoll = glm::radians(_roll);
    float sinRoll = std::sin(radianRoll);
    float cosRoll = std::cos(radianRoll);
    glm::vec3 lookUp = glm::vec3(cosRoll, sinRoll, 0.f);

    _pCamera->SetLookAt(lookFrom + target);
    _pCamera->SetLookUp(lookUp);
    _pCamera->Update();
}

void CameraController::SetPitch(float pitch) {
    _pitch = std::clamp(pitch, -89.9f, +89.9f);
}

void CameraController::SetYaw(float yaw) {
    _yaw = yaw;
}

void CameraController::SetRoll(float roll) {
    _roll = roll;
}

auto CameraController::GetCamera() const -> std::shared_ptr<Camera> {
    return _pCamera;
}
