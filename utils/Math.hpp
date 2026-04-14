#pragma once

#include "MathTypes.hpp"
#include "MatrixMath.hpp"
#include "VectorMath.hpp"

namespace math {

inline Float3 Add(const Float3& left, const Float3& right)
{
  return MakeFloat3(left.x + right.x, left.y + right.y, left.z + right.z);
}

inline Float3 Subtract(const Float3& left, const Float3& right)
{
  return MakeFloat3(left.x - right.x, left.y - right.y, left.z - right.z);
}

inline Float3 Scale(const Float3& value, float scalar)
{
  return MakeFloat3(value.x * scalar, value.y * scalar, value.z * scalar);
}

inline float Dot(const Float3& left, const Float3& right)
{
  return left.x * right.x + left.y * right.y + left.z * right.z;
}

}
