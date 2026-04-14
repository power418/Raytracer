#pragma once

#include "Camera.hpp"
#include "SceneTypes.hpp"
#include "../utils/MathTypes.hpp"

#include <cstdint>
#include <vector>

struct RayTracerSettings
{
  float fieldOfViewDegrees = 60.0f;
  std::uint32_t maxBounces = 1;
  float shadowEpsilon = 1.0e-3f;
  float specularPower = 64.0f;
  float specularStrength = 0.5f;
  float reflectionStrength = 0.2f;
};

class RayTracer
{
public:
  RayTracer();

  void SetSettings(const RayTracerSettings& settings);
  const RayTracerSettings& GetSettings() const;

  void Render(const Camera& camera,
              const CubeState& cubeA,
              const math::Float4& cubeAColor,
              const CubeState& cubeB,
              const math::Float4& cubeBColor,
              std::uint32_t width,
              std::uint32_t height,
              std::vector<std::uint32_t>& outRgba8);

private:
  RayTracerSettings settings_ = {};
};
