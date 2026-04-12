#pragma once

#include "Camera.hpp"
#include "SceneTypes.hpp"
#include "utils/MathTypes.hpp"

#include <cstdint>
#include <memory>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11ShaderResourceView;

class GpuRayTracer
{
public:
  GpuRayTracer();
  ~GpuRayTracer();

  GpuRayTracer(const GpuRayTracer&) = delete;
  GpuRayTracer& operator=(const GpuRayTracer&) = delete;

  void Initialize(ID3D11Device* device, std::uint32_t width, std::uint32_t height);
  void Resize(ID3D11Device* device, std::uint32_t width, std::uint32_t height);
  void Render(ID3D11DeviceContext* context,
              const Camera& camera,
              const CubeState& obstacleCube,
              const math::Float4& obstacleColor,
              const CubeState& playerCube,
              const math::Float4& playerColor);

  ID3D11ShaderResourceView* GetOutputShaderResourceView() const;
  std::uint32_t GetWidth() const;
  std::uint32_t GetHeight() const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
