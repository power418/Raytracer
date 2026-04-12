#pragma once

#define NOMINMAX
#include <Windows.h>

#include "MathTypes.hpp"

namespace input
{
inline bool IsKeyDown(int virtualKey)
{
  return (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
}

inline math::Float2 GetCursorClientPosition(HWND windowHandle)
{
  POINT point = {};
  GetCursorPos(&point);
  ScreenToClient(windowHandle, &point);
  return math::MakeFloat2(static_cast<float>(point.x), static_cast<float>(point.y));
}
}  // namespace input
