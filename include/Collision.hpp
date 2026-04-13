#pragma once

#include "SceneTypes.hpp"

#include <array>

struct OBB
{
  math::Float3 center = {};
  math::Float3 extents = {};
  std::array<math::Float3, 3> axes = {};
};

struct Ray
{
  math::Float3 origin = {};
  math::Float3 direction = {};
};

OBB MakeOBB(const math::Float3& position, const math::Float3& rotation, const math::Float3& extents);
OBB MakeOBB(const CubeState& cube);
bool Intersects(const OBB& a, const OBB& b);
bool IntersectRay(const Ray& ray, const OBB& obb, float* outDistance = nullptr);
