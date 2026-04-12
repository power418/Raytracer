#pragma once

#include <DirectXMath.h>

namespace math
{
using Float2 = DirectX::XMFLOAT2;
using Float3 = DirectX::XMFLOAT3;
using Float4 = DirectX::XMFLOAT4;
using Matrix = DirectX::XMMATRIX;
using Vector = DirectX::XMVECTOR;

inline constexpr float kPi = DirectX::XM_PI;
inline constexpr float kTwoPi = DirectX::XM_2PI;
inline constexpr float kHalfPi = DirectX::XM_PIDIV2;

inline Float2 MakeFloat2(float x = 0.0f, float y = 0.0f)
{
  return Float2(x, y);
}

inline Float3 MakeFloat3(float x = 0.0f, float y = 0.0f, float z = 0.0f)
{
  return Float3(x, y, z);
}

inline Float4 MakeFloat4(float x = 0.0f, float y = 0.0f, float z = 0.0f, float w = 0.0f)
{
  return Float4(x, y, z, w);
}
}  // namespace math
