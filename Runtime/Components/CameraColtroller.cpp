#include "CameraColtroller.h"
#include "Foundation/GameTimer.h"
#include "InputSystem/InputSystem.h"
#include "InputSystem/Keyboard.h"
#include "InputSystem/Mouse.h"
#include "Object/GameObject.h"

CameraController::CameraController()
    : _pitch(0), _yaw(0), _roll(0), _moveState{false}, _mouseRightPress(false), _lastMousePosition(-1, -1) {
}

CameraController::~CameraController() {
}

void CameraController::OnAddToScene() {
    Component::OnAddToScene();
    _postUpdateCallbackHandle = GlobalCallbacks::Get().onPostUpdate.Register(this, &CameraController::OnPostUpdate);

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
    _postUpdateCallbackHandle.Release();
}

class Camera;
void CameraController::OnPostUpdate(GameTimer &timer) {
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
            SetYaw(_yaw - dx);
            _lastMousePosition = POINT{event.x, event.y};
        } /*else if (event._state == MouseState::Wheel) {
            float fovDeviation = event._offset * mouseWheelSensitivity;
            _pCamera->SetFov(std::clamp(cameraData.fov - fovDeviation, 1.f, 89.f));
        }*/
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
        default :
            break;
        }
    }

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
    pTransform->GetWorldTRS(lookFrom, rotation, scale);

    if (advance != 0.f || deviation != 0.f) {
		glm::mat3x3 rotationMatrix = glm::mat3_cast(rotation);
        const glm::vec3 &forward = rotationMatrix[2];
        const glm::vec3 &right = rotationMatrix[0];
        lookFrom= normalize(forward * advance + right * deviation) * (timer.GetDeltaTime() * cameraMoveSpeed);
    }
    // todo: apply roll

    rotation = glm::quat(glm::vec3(_pitch, _yaw, _roll));
    pTransform->SetWorldTRS(lookFrom, rotation, scale);

    std::ranges::fill(_moveState, false);
}
