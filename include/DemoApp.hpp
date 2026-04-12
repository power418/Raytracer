#pragma once

#include "AntiAliasing.hpp"
#include "Camera.hpp"
#include "Renderer.hpp"
#include "SceneTypes.hpp"
#include "Window.hpp"

#include "utils/KeyMap.hpp"

#include <Windows.h>

#include <chrono>
#include <cstdint>

class DemoApp : public IWindowEvents
{
public:
  explicit DemoApp(HINSTANCE instance);
  ~DemoApp() override;

  DemoApp(const DemoApp&) = delete;
  DemoApp& operator=(const DemoApp&) = delete;

  int Run(int showCommand);

  void OnResize(std::uint32_t width, std::uint32_t height) override;
  void OnKeyDown(WPARAM key) override;
  void OnCommand(WPARAM wParam, LPARAM lParam) override;

private:
  void ResetTimer();
  void Tick();
  void Update(float deltaSeconds);
  void HandleAntiAliasingHotkeys(WPARAM key);
  void SetupMenu();
  void UpdateRenderMenuChecks();

private:
  HINSTANCE instance_ = nullptr;
  Window window_;
  Renderer renderer_;
  Camera camera_;
  input::CameraKeyMap cameraKeyMap_;
  input::DemoCubeKeyMap cubeKeyMap_;
  HMENU mainMenu_ = nullptr;
  HMENU renderMenu_ = nullptr;
  CubeState playerCube_ = {};
  CubeState obstacleCube_ = {};
  bool collisionActive_ = false;
  std::chrono::steady_clock::time_point previousFrameTime_;
};
