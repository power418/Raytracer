#pragma once

#include "MathTypes.hpp"
#include "VectorMath.hpp"

namespace math
{
inline Matrix Identity()
{
  return DirectX::XMMatrixIdentity();
}

inline Matrix Scaling(float x, float y, float z)
{
  return DirectX::XMMatrixScaling(x, y, z);
}

inline Matrix RotationRollPitchYaw(float pitch, float yaw, float roll)
{
  return DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
}

inline Matrix Translation(float x, float y, float z)
{
  return DirectX::XMMatrixTranslation(x, y, z);
}

inline Matrix Transpose(Matrix matrix)
{
  return DirectX::XMMatrixTranspose(matrix);
}

inline Matrix LookAtLH(Vector eyePosition, Vector focusPosition, Vector upDirection)
{
  return DirectX::XMMatrixLookAtLH(eyePosition, focusPosition, upDirection);
}

inline Matrix PerspectiveFovLH(float fieldOfViewRadians, float aspectRatio, float nearPlane, float farPlane)
{
  return DirectX::XMMatrixPerspectiveFovLH(fieldOfViewRadians, aspectRatio, nearPlane, farPlane);
}

inline float ToRadians(float degrees)
{
  return DirectX::XMConvertToRadians(degrees);
}

inline Float3 AxisX(Matrix matrix)
{
  return StoreFloat3(matrix.r[0]);
}

inline Float3 AxisY(Matrix matrix)
{
  return StoreFloat3(matrix.r[1]);
}

inline Float3 AxisZ(Matrix matrix)
{
  return StoreFloat3(matrix.r[2]);
}
}  // namespace math
