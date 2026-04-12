#pragma once

#include "MathTypes.hpp"

namespace math
{
inline Vector Load(const Float3& value)
{
  return DirectX::XMLoadFloat3(&value);
}

inline Vector Load(const Float4& value)
{
  return DirectX::XMLoadFloat4(&value);
}

inline Float3 StoreFloat3(Vector value)
{
  Float3 result;
  DirectX::XMStoreFloat3(&result, value);
  return result;
}

inline Float4 StoreFloat4(Vector value)
{
  Float4 result;
  DirectX::XMStoreFloat4(&result, value);
  return result;
}

inline Vector Set(float x, float y, float z, float w)
{
  return DirectX::XMVectorSet(x, y, z, w);
}

inline Vector Subtract(Vector left, Vector right)
{
  return DirectX::XMVectorSubtract(left, right);
}

inline Vector Scale(Vector vector, float scalar)
{
  return DirectX::XMVectorScale(vector, scalar);
}

inline Vector Cross3(Vector left, Vector right)
{
  return DirectX::XMVector3Cross(left, right);
}

inline float Dot3(Vector left, Vector right)
{
  return DirectX::XMVectorGetX(DirectX::XMVector3Dot(left, right));
}

inline float Length3(Vector value)
{
  return DirectX::XMVectorGetX(DirectX::XMVector3Length(value));
}

inline Float3 NormalizeOrFallback(const Float3& value, const Float3& fallback)
{
  const Vector vector = Load(value);
  const float length = Length3(vector);
  if (length < 1.0e-6f)
  {
    return fallback;
  }

  return StoreFloat3(Scale(vector, 1.0f / length));
}
}  // namespace math
