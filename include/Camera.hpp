#pragma once

#include "utils/KeyMap.hpp"
#include "utils/MathTypes.hpp"

#define NOMINMAX
#include <Windows.h>

class Camera
{
public:
  Camera();

  void SetPosition(const math::Float3& position);
  void SetRotation(float yawRadians, float pitchRadians);
  void UpdateFromInput(HWND windowHandle, float deltaSeconds, const input::CameraKeyMap& keyMap);

  math::Matrix GetViewMatrix() const;
  math::Float3 GetPosition() const;
  float GetYaw() const;
  float GetPitch() const;

private:
  math::Float3 GetForwardVector() const;
  math::Float3 GetRightVector() const;

private:
  math::Float3 position_ = {};
  math::Float2 previousMousePosition_ = {};
  float yaw_ = 0.0f;
  float pitch_ = 0.0f;
  bool mouseLookActive_ = false;
};
