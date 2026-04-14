#pragma once

#include "../utils/MathTypes.hpp"

struct CubeState
{
  math::Float3 position = {};
  math::Float3 rotation = {};
  math::Float3 extents = {0.5f, 0.5f, 0.5f};
};
