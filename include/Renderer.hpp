#pragma once

#include "AntiAliasing.hpp"
#include "Camera.hpp"
#include "RenderMode.hpp"
#include "SceneTypes.hpp"

#define NOMINMAX
#include <Windows.h>

#include <cstdint>
#include <memory>

class Renderer
{
public:
  Renderer();
  ~Renderer();

  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;

  void Initialize(HWND windowHandle, std::uint32_t width, std::uint32_t height);
  void Resize(std::uint32_t width, std::uint32_t height);
  void Render(const Camera& camera, const CubeState& obstacleCube, const CubeState& playerCube, bool collisionActive);
  void SetAntiAliasingMode(AntiAliasingMode mode);
  AntiAliasingMode GetAntiAliasingMode() const;
  void SetRenderMode(RenderMode mode);
  RenderMode GetRenderMode() const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
