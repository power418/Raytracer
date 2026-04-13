#pragma once

#include "Camera.hpp"
#include "SceneTypes.hpp"
#include "../utils/Math.hpp"

#define NOMINMAX
#include <Windows.h>

#include <cstdint>
#include <string>
#include <string_view>

class SelectionController
{
public:
  enum class SelectionTarget
  {
    None,
    PlayerCube,
    ObstacleCube,
  };

  enum class TransformMode
  {
    None,
    Move,
    Rotate,
    Scale,
  };

  enum class TransformAxis
  {
    None,
    X,
    Y,
    Z,
  };

  struct TransformSession
  {
    TransformMode mode = TransformMode::None;
    TransformAxis axis = TransformAxis::None;
    CubeState initialCube = {};
    CubeState originalCube = {};
    math::Float3 pivot = {};
    math::Float3 initialHitPoint = {};
    math::Float2 initialCursor = {};
    float initialAxisValue = 0.0f;
  };

public:
  bool HandleKeyDown(WPARAM key,
                     HWND windowHandle,
                     const Camera& camera,
                     std::uint32_t viewportWidth,
                     std::uint32_t viewportHeight,
                     CubeState& playerCube,
                     CubeState& obstacleCube);
  void HandleLeftClick(std::int32_t x,
                       std::int32_t y,
                       HWND windowHandle,
                       const Camera& camera,
                       std::uint32_t viewportWidth,
                       std::uint32_t viewportHeight,
                       CubeState& playerCube,
                       CubeState& obstacleCube);
  void HandleRightClick(CubeState& playerCube, CubeState& obstacleCube);
  void Update(HWND windowHandle,
              const Camera& camera,
              std::uint32_t viewportWidth,
              std::uint32_t viewportHeight,
              CubeState& playerCube,
              CubeState& obstacleCube);

  bool HasActiveTransform() const;
  bool IsPlayerSelected() const;
  bool IsObstacleSelected() const;
  SelectionTarget GetSelectedTarget() const;
  std::wstring BuildWindowTitle(std::wstring_view baseTitle) const;

private:
  void HandleSelectionClick(std::int32_t x,
                            std::int32_t y,
                            const Camera& camera,
                            std::uint32_t viewportWidth,
                            std::uint32_t viewportHeight,
                            const CubeState& playerCube,
                            const CubeState& obstacleCube);
  void StartTransform(TransformMode mode,
                      HWND windowHandle,
                      const Camera& camera,
                      std::uint32_t viewportWidth,
                      std::uint32_t viewportHeight,
                      CubeState& playerCube,
                      CubeState& obstacleCube);
  void RebaseActiveTransform(HWND windowHandle,
                             const Camera& camera,
                             std::uint32_t viewportWidth,
                             std::uint32_t viewportHeight,
                             const CubeState& playerCube,
                             const CubeState& obstacleCube);
  void FinishTransform(bool commitChanges, CubeState& playerCube, CubeState& obstacleCube);
  void UpdateActiveTransform(HWND windowHandle,
                             const Camera& camera,
                             std::uint32_t viewportWidth,
                             std::uint32_t viewportHeight,
                             CubeState& playerCube,
                             CubeState& obstacleCube);

  SelectionTarget PickCube(std::int32_t x,
                           std::int32_t y,
                           const Camera& camera,
                           std::uint32_t viewportWidth,
                           std::uint32_t viewportHeight,
                           const CubeState& playerCube,
                           const CubeState& obstacleCube) const;
  CubeState* GetSelectedCube(CubeState& playerCube, CubeState& obstacleCube);
  const CubeState* GetSelectedCube(const CubeState& playerCube, const CubeState& obstacleCube) const;
  math::Float3 BuildCursorRayDirection(const Camera& camera,
                                       float clientX,
                                       float clientY,
                                       std::uint32_t viewportWidth,
                                       std::uint32_t viewportHeight) const;
  math::Float3 GetCameraForward(const Camera& camera) const;
  math::Float3 GetCameraRight(const Camera& camera) const;
  math::Float3 GetCameraUp(const Camera& camera) const;

private:
  SelectionTarget selectedCube_ = SelectionTarget::None;
  TransformSession transformSession_ = {};
};
