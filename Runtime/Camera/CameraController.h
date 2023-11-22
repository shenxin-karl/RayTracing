#pragma once
#include <memory>
#include <Windows.h>

class Camera;
class GameTimer;
class InputSystem;

class CameraController {
public:
    CameraController(std::shared_ptr<Camera> pCamera);
    void Update(GameTimer &timer);
    void SetPitch(float pitch);
    void SetYaw(float yaw);
    void SetRoll(float roll);
    auto GetCamera() const -> std::shared_ptr<Camera>;
public:
    enum MotionState {
        Forward = 0,
        backward = 1,
        Left = 2,
        Right = 3,
        Down = 4,
        Up = 5,
    };
    float mouseWheelSensitivity = 1.f;
    float mouseMoveSensitivity = 0.2f;    // degrees
    float rollSensitivity = 15.f;         // degrees
    float cameraMoveSpeed = 5.f;          // The number of positions moved in one second
private:
    float _pitch;
    float _yaw;
    float _roll;
    bool _moveState[6] = {false};
    bool _mouseRightPress = false;
    POINT _lastMousePosition = POINT{-1, -1};
    std::shared_ptr<Camera> _pCamera;
};