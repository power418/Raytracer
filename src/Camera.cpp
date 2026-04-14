#include "../include/Camera.hpp"

#include "../utils/Input.hpp"
#include "../utils/Math.hpp"

#include <algorithm>
#include <cmath>

namespace
{
constexpr float kMaxPitchRadians = 1.55334306f;
}

Camera::Camera()
{
  position_ = math::MakeFloat3(0.0f, 2.0f, -6.5f);
  yaw_ = 0.0f;
  pitch_ = -0.29849893f;
}

void Camera::SetPosition(const math::Float3& position)
{
  position_ = position;
}

void Camera::SetRotation(float yawRadians, float pitchRadians)
{
  yaw_ = yawRadians;
  pitch_ = std::clamp(pitchRadians, -kMaxPitchRadians, kMaxPitchRadians);
}

void Camera::UpdateFromInput(HWND windowHandle, float deltaSeconds, const input::CameraKeyMap& keyMap)
{
  float moveSpeed = keyMap.moveSpeed;
  if (input::IsKeyDown(keyMap.fastMoveModifier))
  {
    moveSpeed *= keyMap.fastMoveMultiplier;
  }

  const math::Float3 forward = GetForwardVector();
  const math::Float3 right = GetRightVector();

  if (input::IsKeyDown(keyMap.moveForward))
  {
    position_.x += forward.x * moveSpeed * deltaSeconds;
    position_.y += forward.y * moveSpeed * deltaSeconds;
    position_.z += forward.z * moveSpeed * deltaSeconds;
  }
  if (input::IsKeyDown(keyMap.moveBackward))
  {
    position_.x -= forward.x * moveSpeed * deltaSeconds;
    position_.y -= forward.y * moveSpeed * deltaSeconds;
    position_.z -= forward.z * moveSpeed * deltaSeconds;
  }
  if (input::IsKeyDown(keyMap.moveRight))
  {
    position_.x += right.x * moveSpeed * deltaSeconds;
    position_.y += right.y * moveSpeed * deltaSeconds;
    position_.z += right.z * moveSpeed * deltaSeconds;
  }
  if (input::IsKeyDown(keyMap.moveLeft))
  {
    position_.x -= right.x * moveSpeed * deltaSeconds;
    position_.y -= right.y * moveSpeed * deltaSeconds;
    position_.z -= right.z * moveSpeed * deltaSeconds;
  }
  if (input::IsKeyDown(keyMap.moveUp))
  {
    position_.y += moveSpeed * deltaSeconds;
  }
  if (input::IsKeyDown(keyMap.moveDown))
  {
    position_.y -= moveSpeed * deltaSeconds;
  }

  const bool lookActive = input::IsKeyDown(keyMap.lookButton);
  const math::Float2 cursorPosition = input::GetCursorClientPosition(windowHandle);
  if (lookActive)
  {
    if (!mouseLookActive_)
    {
      previousMousePosition_ = cursorPosition;
      mouseLookActive_ = true;
      return;
    }

    const float deltaX = cursorPosition.x - previousMousePosition_.x;
    const float deltaY = cursorPosition.y - previousMousePosition_.y;

    yaw_ += deltaX * keyMap.mouseLookSpeed;
    pitch_ -= deltaY * keyMap.mouseLookSpeed;
    pitch_ = std::clamp(pitch_, -kMaxPitchRadians, kMaxPitchRadians);
  }
  else
  {
    mouseLookActive_ = false;
  }

  previousMousePosition_ = cursorPosition;
}

math::Matrix Camera::GetViewMatrix() const
{
  const math::Float3 forward = GetForwardVector();
  return math::LookAtLH(
    math::Set(position_.x, position_.y, position_.z, 1.0f),
    math::Set(position_.x + forward.x, position_.y + forward.y, position_.z + forward.z, 1.0f),
    math::Set(0.0f, 1.0f, 0.0f, 0.0f));
}

math::Float3 Camera::GetPosition() const
{
  return position_;
}

float Camera::GetYaw() const
{
  return yaw_;
}

float Camera::GetPitch() const
{
  return pitch_;
}

math::Float3 Camera::GetForwardVector() const
{
  const float cosPitch = std::cos(pitch_);
  return math::NormalizeOrFallback(
    math::MakeFloat3(
      std::sin(yaw_) * cosPitch,
      std::sin(pitch_),
      std::cos(yaw_) * cosPitch),
    math::MakeFloat3(0.0f, 0.0f, 1.0f));
}

math::Float3 Camera::GetRightVector() const
{
  const math::Vector up = math::Set(0.0f, 1.0f, 0.0f, 0.0f);
  const math::Vector forward = math::Load(GetForwardVector());
  return math::NormalizeOrFallback(
    math::StoreFloat3(math::Cross3(up, forward)),
    math::MakeFloat3(1.0f, 0.0f, 0.0f));
}
