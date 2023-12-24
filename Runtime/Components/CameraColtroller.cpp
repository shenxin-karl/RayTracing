#include "CameraColtroller.h"
#include "Foundation/GameTimer.h"
#include "Foundation/Logger.h"
#include "InputSystem/InputSystem.h"
#include "InputSystem/Keyboard.h"
#include "InputSystem/Mouse.h"
#include "Object/GameObject.h"
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Transform.h"

CameraController::CameraController()
    : _pitch(0), _yaw(0), _roll(0), _moveState{false}, _mouseRightPress(false), _lastMousePosition(-1, -1) {
    SetTickType(TickType::ePostUpdate);
}

CameraController::~CameraController() {
}

void CameraController::OnAddToScene() {
    Component::OnAddToScene();

    Transform *pTransform = GetGameObject()->GetComponent<Transform>();
    if (pTransform == nullptr) {
        return;
    }

    glm::vec3 translation;
    glm::quat rotation;
    glm::vec3 scale;
    pTransform->GetWorldTRS(translation, rotation, scale);

    glm::vec3 euler = glm::degrees(glm::eulerAngles(rotation));
    _pitch = euler.x;
    _yaw = euler.y;
    _roll = euler.z;
}

void CameraController::OnRemoveFormScene() {
    Component::OnRemoveFormScene();
}

class Camera;
void CameraController::OnPostUpdate() {
    Transform *pTransform = GetGameObject()->GetComponent<Transform>();
    if (pTransform == nullptr || !GetGameObject()->HasComponent(::GetTypeID<Camera>())) {
        return;
    }

    InputSystem *pInputSystem = InputSystem::GetInstance();
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
            SetYaw(_yaw + dx);
            _lastMousePosition = POINT{event.x, event.y};
        } /*else if (event._state == MouseState::Wheel) {
            float fovDeviation = event._offset * mouseWheelSensitivity;
            _pCamera->SetFov(std::clamp(cameraData.fov - fovDeviation, 1.f, 89.f));
        }*/
    }

    _moveState[Forward] = pInputSystem->pKeyboard->IsKeyPressed('W');
    _moveState[backward] = pInputSystem->pKeyboard->IsKeyPressed('S');
    _moveState[Left] = pInputSystem->pKeyboard->IsKeyPressed('A');
    _moveState[Right] = pInputSystem->pKeyboard->IsKeyPressed('D');
    _moveState[Down] = pInputSystem->pKeyboard->IsKeyPressed('Q');
    _moveState[Up] = pInputSystem->pKeyboard->IsKeyPressed('E');

    float advance = 0.f;
    float deviation = 0.f;
    [[maybe_unused]] float elevationRise = 0.f;
    advance += static_cast<float>(_moveState[Forward]);
    advance -= static_cast<float>(_moveState[backward]);
    deviation += static_cast<float>(_moveState[Right]);
    deviation -= static_cast<float>(_moveState[Left]);
    elevationRise += static_cast<float>(_moveState[Up]);
    elevationRise -= static_cast<float>(_moveState[Down]);

    glm::vec3 lookFrom;
    glm::quat rotation;
    glm::vec3 scale;
    pTransform->GetLocalTRS(lookFrom, rotation, scale);

    if (advance != 0.f || deviation != 0.f || elevationRise != 0.f) {
        glm::vec3 right = {};
        glm::vec3 up = {};
        glm::vec3 forward = {};
        glm::Quaternion2BasisAxis(rotation, right, up, forward);

        float moveStep = GameTimer::Get().GetDeltaTime() * cameraMoveSpeed;
        glm::vec3 offsetX = right * deviation;
        glm::vec3 offsetY = up * elevationRise;
        glm::vec3 offsetZ = forward * advance;
        lookFrom += normalize(offsetX + offsetY + offsetZ) * moveStep;
    }
    // todo: apply roll

    glm::vec3 front;
    front.y = std::sin(glm::radians(_pitch));
    front.x = std::cos(glm::radians(_pitch)) * std::sin(glm::radians(_yaw));
    front.z = std::cos(glm::radians(_pitch)) * std::cos(glm::radians(_yaw));
    rotation = glm::Direction2Quaternion(front);
    pTransform->SetLocalTRS(lookFrom, rotation, scale);
    std::ranges::fill(_moveState, false);
}
