#pragma once

#include <cstdint>

enum class RenderMode : std::uint32_t
{
  Raster = 0,
  RayTrace = 1,
};
