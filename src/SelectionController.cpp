#include "SelectionController.hpp"

#include "Collision.hpp"
#include "utils/Input.hpp"
#include "utils/Math.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
constexpr float kCameraFieldOfViewRadians = 1.04719755f;
constexpr float kRotationSensitivity = 0.01f;
constexpr float kScaleSensitivity = 0.01f;
constexpr float kMinimumExtent = 0.1f;

math::Float3 Add(const math::Float3& left, const math::Float3& right)
{
  return math::MakeFloat3(left.x + right.x, left.y + right.y, left.z + right.z);
}

math::Float3 Subtract(const math::Float3& left, const math::Float3& right)
{
  return math::MakeFloat3(left.x - right.x, left.y - right.y, left.z - right.z);
}

math::Float3 Scale(const math::Float3& value, float scalar)
{
  return math::MakeFloat3(value.x * scalar, value.y * scalar, value.z * scalar);
}

float Dot(const math::Float3& left, const math::Float3& right)
{
  return left.x * right.x + left.y * right.y + left.z * right.z;
}

math::Float3 GetAxisDirection(SelectionController::TransformAxis axis)
{
  switch (axis)
  {
  case SelectionController::TransformAxis::X:
    return math::MakeFloat3(1.0f, 0.0f, 0.0f);
  case SelectionController::TransformAxis::Y:
    return math::MakeFloat3(0.0f, 1.0f, 0.0f);
  case SelectionController::TransformAxis::Z:
    return math::MakeFloat3(0.0f, 0.0f, 1.0f);
  default:
    return math::MakeFloat3(0.0f, 0.0f, 0.0f);
  }
}

bool IntersectRayWithPlane(const Ray& ray,
                           const math::Float3& planePoint,
                           const math::Float3& planeNormal,
                           math::Float3* outHitPoint)
{
  const float denominator = Dot(ray.direction, planeNormal);
  if (std::fabs(denominator) < 1.0e-6f)
  {
    return false;
  }

  const float distance = Dot(Subtract(planePoint, ray.origin), planeNormal) / denominator;
  if (distance <= 1.0e-4f)
  {
    return false;
  }

  if (outHitPoint)
  {
    *outHitPoint = Add(ray.origin, Scale(ray.direction, distance));
  }

  return true;
}

math::Float3 BuildAxisPlaneNormal(const math::Float3& axisDirection,
                                  const math::Float3& cameraForward,
                                  const math::Float3& cameraRight,
                                  const math::Float3& cameraUp)
{
  math::Float3 planeNormal =
      Subtract(cameraForward, Scale(axisDirection, Dot(cameraForward, axisDirection)));
  planeNormal = math::NormalizeOrFallback(planeNormal, math::MakeFloat3(0.0f, 0.0f, 0.0f));
  if (math::Length3(math::Load(planeNormal)) > 1.0e-4f)
  {
    return planeNormal;
  }

  planeNormal = Subtract(cameraRight, Scale(axisDirection, Dot(cameraRight, axisDirection)));
  planeNormal = math::NormalizeOrFallback(planeNormal, math::MakeFloat3(0.0f, 0.0f, 0.0f));
  if (math::Length3(math::Load(planeNormal)) > 1.0e-4f)
  {
    return planeNormal;
  }

  planeNormal = Subtract(cameraUp, Scale(axisDirection, Dot(cameraUp, axisDirection)));
  return math::NormalizeOrFallback(planeNormal, math::MakeFloat3(0.0f, 1.0f, 0.0f));
}

const wchar_t* ToLabel(SelectionController::SelectionTarget target)
{
  switch (target)
  {
  case SelectionController::SelectionTarget::PlayerCube:
    return L"Player Cube";
  case SelectionController::SelectionTarget::ObstacleCube:
    return L"Obstacle Cube";
  default:
    return L"None";
  }
}

const wchar_t* ToLabel(SelectionController::TransformMode mode)
{
  switch (mode)
  {
  case SelectionController::TransformMode::Move:
    return L"Move";
  case SelectionController::TransformMode::Rotate:
    return L"Rotate";
  case SelectionController::TransformMode::Scale:
    return L"Scale";
  default:
    return L"Idle";
  }
}

const wchar_t* ToLabel(SelectionController::TransformAxis axis)
{
  switch (axis)
  {
  case SelectionController::TransformAxis::X:
    return L"X";
  case SelectionController::TransformAxis::Y:
    return L"Y";
  case SelectionController::TransformAxis::Z:
    return L"Z";
  default:
    return L"Free";
  }
}
}  // namespace

bool SelectionController::HandleKeyDown(WPARAM key,
                                        HWND windowHandle,
                                        const Camera& camera,
                                        std::uint32_t viewportWidth,
                                        std::uint32_t viewportHeight,
                                        CubeState& playerCube,
                                        CubeState& obstacleCube)
{
  if (transformSession_.mode != TransformMode::None)
  {
    if (key == VK_ESCAPE)
    {
      FinishTransform(false, playerCube, obstacleCube);
      return true;
    }

    if (key == VK_RETURN)
    {
      FinishTransform(true, playerCube, obstacleCube);
      return true;
    }
  }

  switch (key)
  {
  case 'G':
    StartTransform(TransformMode::Move, windowHandle, camera, viewportWidth, viewportHeight, playerCube, obstacleCube);
    return true;
  case 'R':
    StartTransform(TransformMode::Rotate, windowHandle, camera, viewportWidth, viewportHeight, playerCube, obstacleCube);
    return true;
  case 'S':
    StartTransform(TransformMode::Scale, windowHandle, camera, viewportWidth, viewportHeight, playerCube, obstacleCube);
    return true;
  case 'X':
  case 'Y':
  case 'Z':
    if (transformSession_.mode == TransformMode::None)
    {
      return false;
    }

    {
      const TransformAxis requestedAxis = (key == 'X') ? TransformAxis::X : (key == 'Y' ? TransformAxis::Y : TransformAxis::Z);
      transformSession_.axis = (transformSession_.axis == requestedAxis) ? TransformAxis::None : requestedAxis;
      RebaseActiveTransform(windowHandle, camera, viewportWidth, viewportHeight, playerCube, obstacleCube);
    }
    return true;
  default:
    return false;
  }
}

void SelectionController::HandleLeftClick(std::int32_t x,
                                          std::int32_t y,
                                          HWND windowHandle,
                                          const Camera& camera,
                                          std::uint32_t viewportWidth,
                                          std::uint32_t viewportHeight,
                                          CubeState& playerCube,
                                          CubeState& obstacleCube)
{
  if (transformSession_.mode != TransformMode::None)
  {
    FinishTransform(true, playerCube, obstacleCube);
    return;
  }

  HandleSelectionClick(x, y, camera, viewportWidth, viewportHeight, playerCube, obstacleCube);
}

void SelectionController::HandleRightClick(CubeState& playerCube, CubeState& obstacleCube)
{
  if (transformSession_.mode != TransformMode::None)
  {
    FinishTransform(false, playerCube, obstacleCube);
  }
}

void SelectionController::Update(HWND windowHandle,
                                 const Camera& camera,
                                 std::uint32_t viewportWidth,
                                 std::uint32_t viewportHeight,
                                 CubeState& playerCube,
                                 CubeState& obstacleCube)
{
  if (transformSession_.mode != TransformMode::None)
  {
    UpdateActiveTransform(windowHandle, camera, viewportWidth, viewportHeight, playerCube, obstacleCube);
  }
}

bool SelectionController::HasActiveTransform() const
{
  return transformSession_.mode != TransformMode::None;
}

bool SelectionController::IsPlayerSelected() const
{
  return selectedCube_ == SelectionTarget::PlayerCube;
}

bool SelectionController::IsObstacleSelected() const
{
  return selectedCube_ == SelectionTarget::ObstacleCube;
}

SelectionController::SelectionTarget SelectionController::GetSelectedTarget() const
{
  return selectedCube_;
}

std::wstring SelectionController::BuildWindowTitle(std::wstring_view baseTitle) const
{
  std::wstring title(baseTitle);
  title += L" | Selected: ";
  title += ToLabel(selectedCube_);
  title += L" | Mode: ";
  title += ToLabel(transformSession_.mode);
  title += L" | Axis: ";
  title += ToLabel(transformSession_.axis);

  if (transformSession_.mode == TransformMode::None)
  {
    title += L" | LMB Select | G Move | R Rotate | S Scale";
  }
  else
  {
    title += L" | X/Y/Z Constraint | Enter/LMB Confirm | Esc/RMB Cancel";
  }

  return title;
}

void SelectionController::HandleSelectionClick(std::int32_t x,
                                               std::int32_t y,
                                               const Camera& camera,
                                               std::uint32_t viewportWidth,
                                               std::uint32_t viewportHeight,
                                               const CubeState& playerCube,
                                               const CubeState& obstacleCube)
{
  selectedCube_ = PickCube(x, y, camera, viewportWidth, viewportHeight, playerCube, obstacleCube);
}

void SelectionController::StartTransform(TransformMode mode,
                                         HWND windowHandle,
                                         const Camera& camera,
                                         std::uint32_t viewportWidth,
                                         std::uint32_t viewportHeight,
                                         CubeState& playerCube,
                                         CubeState& obstacleCube)
{
  CubeState* cube = GetSelectedCube(playerCube, obstacleCube);
  if (!cube)
  {
    return;
  }

  transformSession_ = {};
  transformSession_.mode = mode;
  transformSession_.initialCube = *cube;
  RebaseActiveTransform(windowHandle, camera, viewportWidth, viewportHeight, playerCube, obstacleCube);
}

void SelectionController::RebaseActiveTransform(HWND windowHandle,
                                                const Camera& camera,
                                                std::uint32_t viewportWidth,
                                                std::uint32_t viewportHeight,
                                                const CubeState& playerCube,
                                                const CubeState& obstacleCube)
{
  const CubeState* cube = GetSelectedCube(playerCube, obstacleCube);
  if (!cube || transformSession_.mode == TransformMode::None)
  {
    return;
  }

  transformSession_.originalCube = *cube;
  transformSession_.pivot = cube->position;
  transformSession_.initialCursor = input::GetCursorClientPosition(windowHandle);
  transformSession_.initialHitPoint = cube->position;
  transformSession_.initialAxisValue = 0.0f;

  if (transformSession_.mode == TransformMode::Move)
  {
    const math::Float2 cursor = transformSession_.initialCursor;
    const Ray ray = {camera.GetPosition(),
                     BuildCursorRayDirection(camera, cursor.x, cursor.y, viewportWidth, viewportHeight)};

    math::Float3 planeNormal = GetCameraForward(camera);
    if (transformSession_.axis != TransformAxis::None)
    {
      planeNormal = BuildAxisPlaneNormal(GetAxisDirection(transformSession_.axis),
                                         GetCameraForward(camera),
                                         GetCameraRight(camera),
                                         GetCameraUp(camera));
    }

    math::Float3 hitPoint = cube->position;
    if (IntersectRayWithPlane(ray, cube->position, planeNormal, &hitPoint))
    {
      transformSession_.initialHitPoint = hitPoint;
      if (transformSession_.axis != TransformAxis::None)
      {
        transformSession_.initialAxisValue =
            Dot(Subtract(hitPoint, cube->position), GetAxisDirection(transformSession_.axis));
      }
    }
  }
}

void SelectionController::FinishTransform(bool commitChanges, CubeState& playerCube, CubeState& obstacleCube)
{
  CubeState* cube = GetSelectedCube(playerCube, obstacleCube);
  if (cube && !commitChanges)
  {
    *cube = transformSession_.initialCube;
  }

  transformSession_ = {};
}

void SelectionController::UpdateActiveTransform(HWND windowHandle,
                                                const Camera& camera,
                                                std::uint32_t viewportWidth,
                                                std::uint32_t viewportHeight,
                                                CubeState& playerCube,
                                                CubeState& obstacleCube)
{
  CubeState* cube = GetSelectedCube(playerCube, obstacleCube);
  if (!cube)
  {
    transformSession_ = {};
    return;
  }

  const math::Float2 cursor = input::GetCursorClientPosition(windowHandle);
  const float deltaX = cursor.x - transformSession_.initialCursor.x;
  const float deltaY = transformSession_.initialCursor.y - cursor.y;

  if (transformSession_.mode == TransformMode::Move)
  {
    const Ray ray = {camera.GetPosition(),
                     BuildCursorRayDirection(camera, cursor.x, cursor.y, viewportWidth, viewportHeight)};
    math::Float3 planeNormal = GetCameraForward(camera);
    math::Float3 axisDirection = math::MakeFloat3(0.0f, 0.0f, 0.0f);

    if (transformSession_.axis != TransformAxis::None)
    {
      axisDirection = GetAxisDirection(transformSession_.axis);
      planeNormal = BuildAxisPlaneNormal(axisDirection,
                                         GetCameraForward(camera),
                                         GetCameraRight(camera),
                                         GetCameraUp(camera));
    }

    math::Float3 hitPoint = {};
    if (!IntersectRayWithPlane(ray, transformSession_.pivot, planeNormal, &hitPoint))
    {
      return;
    }

    if (transformSession_.axis == TransformAxis::None)
    {
      cube->position = Add(transformSession_.originalCube.position,
                           Subtract(hitPoint, transformSession_.initialHitPoint));
    }
    else
    {
      const float currentAxisValue = Dot(Subtract(hitPoint, transformSession_.pivot), axisDirection);
      const float deltaAxis = currentAxisValue - transformSession_.initialAxisValue;
      cube->position = Add(transformSession_.originalCube.position, Scale(axisDirection, deltaAxis));
    }

    return;
  }

  if (transformSession_.mode == TransformMode::Rotate)
  {
    const float angle = (deltaX + deltaY * 0.5f) * kRotationSensitivity;
    math::Float3 axisWeights = GetCameraForward(camera);
    if (transformSession_.axis != TransformAxis::None)
    {
      axisWeights = GetAxisDirection(transformSession_.axis);
    }

    cube->rotation.x = transformSession_.originalCube.rotation.x + axisWeights.x * angle;
    cube->rotation.y = transformSession_.originalCube.rotation.y + axisWeights.y * angle;
    cube->rotation.z = transformSession_.originalCube.rotation.z + axisWeights.z * angle;
    return;
  }

  if (transformSession_.mode == TransformMode::Scale)
  {
    const float scaleFactor = std::max(0.1f, 1.0f + (deltaX + deltaY) * kScaleSensitivity);
    if (transformSession_.axis == TransformAxis::None)
    {
      cube->extents.x = std::max(kMinimumExtent, transformSession_.originalCube.extents.x * scaleFactor);
      cube->extents.y = std::max(kMinimumExtent, transformSession_.originalCube.extents.y * scaleFactor);
      cube->extents.z = std::max(kMinimumExtent, transformSession_.originalCube.extents.z * scaleFactor);
      return;
    }

    cube->extents = transformSession_.originalCube.extents;
    if (transformSession_.axis == TransformAxis::X)
    {
      cube->extents.x = std::max(kMinimumExtent, transformSession_.originalCube.extents.x * scaleFactor);
    }
    else if (transformSession_.axis == TransformAxis::Y)
    {
      cube->extents.y = std::max(kMinimumExtent, transformSession_.originalCube.extents.y * scaleFactor);
    }
    else if (transformSession_.axis == TransformAxis::Z)
    {
      cube->extents.z = std::max(kMinimumExtent, transformSession_.originalCube.extents.z * scaleFactor);
    }
  }
}

SelectionController::SelectionTarget SelectionController::PickCube(std::int32_t x,
                                                                   std::int32_t y,
                                                                   const Camera& camera,
                                                                   std::uint32_t viewportWidth,
                                                                   std::uint32_t viewportHeight,
                                                                   const CubeState& playerCube,
                                                                   const CubeState& obstacleCube) const
{
  if (viewportWidth == 0 || viewportHeight == 0)
  {
    return SelectionTarget::None;
  }

  const Ray ray = {camera.GetPosition(),
                   BuildCursorRayDirection(camera, static_cast<float>(x), static_cast<float>(y), viewportWidth, viewportHeight)};

  float closestDistance = std::numeric_limits<float>::max();
  SelectionTarget bestTarget = SelectionTarget::None;

  float hitDistance = 0.0f;
  if (IntersectRay(ray, MakeOBB(playerCube), &hitDistance) && hitDistance < closestDistance)
  {
    closestDistance = hitDistance;
    bestTarget = SelectionTarget::PlayerCube;
  }

  if (IntersectRay(ray, MakeOBB(obstacleCube), &hitDistance) && hitDistance < closestDistance)
  {
    bestTarget = SelectionTarget::ObstacleCube;
  }

  return bestTarget;
}

CubeState* SelectionController::GetSelectedCube(CubeState& playerCube, CubeState& obstacleCube)
{
  switch (selectedCube_)
  {
  case SelectionTarget::PlayerCube:
    return &playerCube;
  case SelectionTarget::ObstacleCube:
    return &obstacleCube;
  default:
    return nullptr;
  }
}

const CubeState* SelectionController::GetSelectedCube(const CubeState& playerCube, const CubeState& obstacleCube) const
{
  switch (selectedCube_)
  {
  case SelectionTarget::PlayerCube:
    return &playerCube;
  case SelectionTarget::ObstacleCube:
    return &obstacleCube;
  default:
    return nullptr;
  }
}

math::Float3 SelectionController::BuildCursorRayDirection(const Camera& camera,
                                                          float clientX,
                                                          float clientY,
                                                          std::uint32_t viewportWidth,
                                                          std::uint32_t viewportHeight) const
{
  const float width = static_cast<float>(std::max(viewportWidth, 1u));
  const float height = static_cast<float>(std::max(viewportHeight, 1u));
  const float aspect = width / height;
  const float tanHalfFov = std::tan(kCameraFieldOfViewRadians * 0.5f);
  const float normalizedX = ((clientX / width) * 2.0f - 1.0f) * aspect * tanHalfFov;
  const float normalizedY = (1.0f - (clientY / height) * 2.0f) * tanHalfFov;

  const math::Float3 forward = GetCameraForward(camera);
  const math::Float3 right = GetCameraRight(camera);
  const math::Float3 up = GetCameraUp(camera);

  return math::NormalizeOrFallback(
      Add(Add(forward, Scale(right, normalizedX)), Scale(up, normalizedY)),
      forward);
}

math::Float3 SelectionController::GetCameraForward(const Camera& camera) const
{
  const float yaw = camera.GetYaw();
  const float pitch = camera.GetPitch();
  const float cosPitch = std::cos(pitch);
  return math::NormalizeOrFallback(
      math::MakeFloat3(std::sin(yaw) * cosPitch, std::sin(pitch), std::cos(yaw) * cosPitch),
      math::MakeFloat3(0.0f, 0.0f, 1.0f));
}

math::Float3 SelectionController::GetCameraRight(const Camera& camera) const
{
  return math::NormalizeOrFallback(
      math::StoreFloat3(math::Cross3(math::Set(0.0f, 1.0f, 0.0f, 0.0f), math::Load(GetCameraForward(camera)))),
      math::MakeFloat3(1.0f, 0.0f, 0.0f));
}

math::Float3 SelectionController::GetCameraUp(const Camera& camera) const
{
  return math::NormalizeOrFallback(
      math::StoreFloat3(math::Cross3(math::Load(GetCameraForward(camera)), math::Load(GetCameraRight(camera)))),
      math::MakeFloat3(0.0f, 1.0f, 0.0f));
}
