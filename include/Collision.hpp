#pragma once

#include "SceneTypes.hpp"

#include <array>

struct OBB
{
  math::Float3 center = {};
  math::Float3 extents = {};
  std::array<math::Float3, 3> axes = {};
};

OBB MakeOBB(const math::Float3& position, const math::Float3& rotation, const math::Float3& extents);
OBB MakeOBB(const CubeState& cube);
bool Intersects(const OBB& a, const OBB& b);
