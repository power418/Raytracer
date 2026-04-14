#include "../include/Collision.hpp"
#include "../utils/Math.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

OBB MakeOBB(const math::Float3& position, const math::Float3& rotation, const math::Float3& extents)
{
  const math::Matrix rotationMatrix = math::RotationRollPitchYaw(
    rotation.x,
    rotation.y,
    rotation.z);

  const math::Float3 axisX = math::AxisX(rotationMatrix);
  const math::Float3 axisY = math::AxisY(rotationMatrix);
  const math::Float3 axisZ = math::AxisZ(rotationMatrix);

  OBB result = {};
  result.center = position;
  result.extents = extents;
  result.axes = {
    math::NormalizeOrFallback(axisX, math::MakeFloat3(1.0f, 0.0f, 0.0f)),
    math::NormalizeOrFallback(axisY, math::MakeFloat3(0.0f, 1.0f, 0.0f)),
    math::NormalizeOrFallback(axisZ, math::MakeFloat3(0.0f, 0.0f, 1.0f)),
  };
  return result;
}

OBB MakeOBB(const CubeState& cube)
{
  return MakeOBB(cube.position, cube.rotation, cube.extents);
}

bool Intersects(const OBB& a, const OBB& b)
{
  constexpr float epsilon = 1.0e-4f;

  float rotation[3][3] = {};
  float absRotation[3][3] = {};
  float translation[3] = {};

  const math::Vector aCenter = math::Load(a.center);
  const math::Vector bCenter = math::Load(b.center);
  const math::Vector delta = math::Subtract(bCenter, aCenter);

  for (int i = 0; i < 3; ++i)
  {
    const math::Vector aAxis = math::Load(a.axes[i]);
    translation[i] = math::Dot3(delta, aAxis);

    for (int j = 0; j < 3; ++j)
    {
      const math::Vector bAxis = math::Load(b.axes[j]);
      rotation[i][j] = math::Dot3(aAxis, bAxis);
      absRotation[i][j] = std::fabs(rotation[i][j]) + epsilon;
    }
  }

  for (int i = 0; i < 3; ++i)
  {
    const float radiusA = (&a.extents.x)[i];
    const float radiusB = b.extents.x * absRotation[i][0] +
                          b.extents.y * absRotation[i][1] +
                          b.extents.z * absRotation[i][2];
    if (std::fabs(translation[i]) > radiusA + radiusB)
    {
      return false;
    }
  }

  for (int j = 0; j < 3; ++j)
  {
    const float radiusA = a.extents.x * absRotation[0][j] +
                          a.extents.y * absRotation[1][j] +
                          a.extents.z * absRotation[2][j];
    const float radiusB = (&b.extents.x)[j];
    const float projection = std::fabs(translation[0] * rotation[0][j] +
                                       translation[1] * rotation[1][j] +
                                       translation[2] * rotation[2][j]);
    if (projection > radiusA + radiusB)
    {
      return false;
    }
  }

  for (int i = 0; i < 3; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      const int i1 = (i + 1) % 3;
      const int i2 = (i + 2) % 3;
      const int j1 = (j + 1) % 3;
      const int j2 = (j + 2) % 3;

      const float radiusA = (&a.extents.x)[i1] * absRotation[i2][j] +
                            (&a.extents.x)[i2] * absRotation[i1][j];
      const float radiusB = (&b.extents.x)[j1] * absRotation[i][j2] +
                            (&b.extents.x)[j2] * absRotation[i][j1];
      const float projection = std::fabs(translation[i2] * rotation[i1][j] -
                                         translation[i1] * rotation[i2][j]);
      if (projection > radiusA + radiusB)
      {
        return false;
      }
    }
  }

  return true;
}

bool IntersectRay(const Ray& ray, const OBB& obb, float* outDistance)
{
  constexpr float epsilon = 1.0e-6f;

  const math::Vector rayOrigin = math::Load(ray.origin);
  const math::Vector obbCenter = math::Load(obb.center);
  const math::Vector delta = math::Subtract(rayOrigin, obbCenter);

  float tMin = 0.0f;
  float tMax = 1.0e20f;

  for (int axisIndex = 0; axisIndex < 3; ++axisIndex)
  {
    const math::Vector axis = math::Load(obb.axes[axisIndex]);
    const float originProjection = math::Dot3(delta, axis);
    const float directionProjection = math::Dot3(math::Load(ray.direction), axis);
    const float extent = (&obb.extents.x)[axisIndex];

    if (std::fabs(directionProjection) < epsilon)
    {
      if (originProjection < -extent || originProjection > extent)
      {
        return false;
      }

      continue;
    }

    float t1 = (-extent - originProjection) / directionProjection;
    float t2 = (extent - originProjection) / directionProjection;
    if (t1 > t2)
    {
      std::swap(t1, t2);
    }

    tMin = std::max(tMin, t1);
    tMax = std::min(tMax, t2);
    if (tMin > tMax)
    {
      return false;
    }
  }

  const float distance = tMin > epsilon ? tMin : tMax;
  if (distance <= epsilon)
  {
    return false;
  }

  if (outDistance)
  {
    *outDistance = distance;
  }

  return true;
}
