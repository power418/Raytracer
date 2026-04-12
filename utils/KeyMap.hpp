#pragma once

#define NOMINMAX
#include <Windows.h>

namespace input
{
struct CameraKeyMap
{
  int moveForward = 'W';
  int moveBackward = 'S';
  int moveLeft = 'A';
  int moveRight = 'D';
  int moveDown = 'Q';
  int moveUp = 'E';
  int fastMoveModifier = VK_SHIFT;
  int lookButton = VK_RBUTTON;

  float moveSpeed = 4.5f;
  float fastMoveMultiplier = 2.5f;
  float mouseLookSpeed = 0.0035f;
};

struct DemoCubeKeyMap
{
  int moveLeft = VK_LEFT;
  int moveRight = VK_RIGHT;
  int moveForward = VK_UP;
  int moveBackward = VK_DOWN;
  float moveSpeed = 2.4f;
};

inline CameraKeyMap MakeUnrealStyleCameraKeyMap()
{
  return CameraKeyMap{};
}

inline DemoCubeKeyMap MakeDemoCubeKeyMap()
{
  return DemoCubeKeyMap{};
}
}  // namespace input
