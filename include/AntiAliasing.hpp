#pragma once

#include <cstdint>

enum class AntiAliasingMode : std::uint32_t
{
  None = 0,
  FXAA = 1,
  SMAA = 2,
};
