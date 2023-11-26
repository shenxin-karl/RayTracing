#pragma once
#include "Component.h"
#include <Windows.h>

class CameraController : public Component {
    DECLARE_CLASS(CameraController);
public:
    CameraController();
    ~CameraController() override;
private:
    void OnAddToScene() override;
    void OnRemoveFormScene() override;
    void OnPostUpdate(GameTimer &timer);
    void SetPitch(float pitch) {
        _pitch = std::clamp(pitch, -89.9f, +89.9f);
    }
    void SetYaw(float yaw) {
        _yaw = yaw;
    }
    void SetRoll(float roll) {
        _roll = roll;
    }
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
    // clang-format off
	float           _pitch;
	float           _yaw;
	float           _roll;
	bool            _moveState[6];       
    bool            _mouseRightPress;    
    POINT           _lastMousePosition;
    CallbackHandle  _postUpdateCallbackHandle;
    // clang-format on
};
