#include "DemoApp.hpp"

#include "Collision.hpp"
#include "utils/Input.hpp"
#include "utils/Math.hpp"

#include <algorithm>
#include <cstdio>

namespace
{
constexpr wchar_t kWindowTitle[] = L"DirectX 11 Demo Application";
constexpr math::Float3 kCameraCollisionExtents = {0.35f, 0.65f, 0.35f};
constexpr UINT kMenuRenderRaster = 40001;
constexpr UINT kMenuRenderRayTrace = 40002;
}  // namespace

DemoApp::DemoApp(HINSTANCE instance)
  : instance_(instance),
    window_(instance_, *this, kWindowTitle, 1280, 720),
    cameraKeyMap_(input::MakeUnrealStyleCameraKeyMap()),
    cubeKeyMap_(input::MakeDemoCubeKeyMap())
{
  std::printf("[DemoApp] Constructing DemoApp...\n");
  playerCube_.position = math::MakeFloat3(-2.2f, 0.0f, 0.0f);
  obstacleCube_.position = math::MakeFloat3(0.0f, 0.0f, 0.0f);
  std::printf("[DemoApp] Player cube at (%.1f, %.1f, %.1f)\n",
              playerCube_.position.x, playerCube_.position.y, playerCube_.position.z);
  std::printf("[DemoApp] Obstacle cube at (%.1f, %.1f, %.1f)\n",
              obstacleCube_.position.x, obstacleCube_.position.y, obstacleCube_.position.z);
  std::printf("[DemoApp] Construction complete\n");
}

DemoApp::~DemoApp() = default;

int DemoApp::Run(int showCommand)
{
  std::printf("[DemoApp] Run() called\n");
  std::printf("[DemoApp] Showing window...\n");
  window_.Show(showCommand);
  std::printf("[DemoApp] Window shown successfully\n");

  std::printf("[DemoApp] Initializing renderer (%ux%u)...\n",
              window_.GetClientWidth(), window_.GetClientHeight());
  renderer_.Initialize(window_.GetHandle(), window_.GetClientWidth(), window_.GetClientHeight());
  std::printf("[DemoApp] Renderer initialized successfully\n");

  SetupMenu();
  std::printf("[DemoApp] Menu setup complete\n");
  UpdateWindowTitle();
  ResetTimer();

  std::printf("[DemoApp] Entering main loop...\n");
  std::uint64_t frameCount = 0;
  while (window_.ProcessMessages())
  {
    Tick();
    ++frameCount;
    if (frameCount % 600 == 0)
    {
      std::printf("[DemoApp] Frame %llu rendered\n", frameCount);
    }
  }

  std::printf("[DemoApp] Main loop exited after %llu frames\n", frameCount);
  return 0;
}

void DemoApp::OnResize(std::uint32_t width, std::uint32_t height)
{
  std::printf("[DemoApp] OnResize(%u, %u)\n", width, height);
  renderer_.Resize(width, height);
}

void DemoApp::OnKeyDown(WPARAM key)
{
  std::printf("[DemoApp] OnKeyDown(0x%02llX)\n", static_cast<unsigned long long>(key));
  if (selectionController_.HandleKeyDown(key,
                                         window_.GetHandle(),
                                         camera_,
                                         window_.GetClientWidth(),
                                         window_.GetClientHeight(),
                                         playerCube_,
                                         obstacleCube_))
  {
    UpdateWindowTitle();
    return;
  }

  if (key == VK_ESCAPE)
  {
    DestroyWindow(window_.GetHandle());
    return;
  }

  HandleAntiAliasingHotkeys(key);
}

void DemoApp::OnLeftMouseDown(std::int32_t x, std::int32_t y)
{
  std::printf("[DemoApp] OnLeftMouseDown(%d, %d)\n", x, y);
  selectionController_.HandleLeftClick(x,
                                       y,
                                       window_.GetHandle(),
                                       camera_,
                                       window_.GetClientWidth(),
                                       window_.GetClientHeight(),
                                       playerCube_,
                                       obstacleCube_);
  UpdateWindowTitle();
}

void DemoApp::OnRightMouseDown(std::int32_t x, std::int32_t y)
{
  std::printf("[DemoApp] OnRightMouseDown(%d, %d)\n", x, y);
  selectionController_.HandleRightClick(playerCube_, obstacleCube_);
  UpdateWindowTitle();
}

void DemoApp::OnCommand(WPARAM wParam, LPARAM)
{
  std::printf("[DemoApp] OnCommand(0x%04llX)\n", static_cast<unsigned long long>(LOWORD(wParam)));
  const UINT commandId = LOWORD(wParam);
  switch (commandId)
  {
  case kMenuRenderRaster:
    renderer_.SetRenderMode(RenderMode::Raster);
    UpdateRenderMenuChecks();
    break;
  case kMenuRenderRayTrace:
    renderer_.SetRenderMode(RenderMode::RayTrace);
    UpdateRenderMenuChecks();
    break;
  default:
    break;
  }
}

void DemoApp::ResetTimer()
{
  previousFrameTime_ = std::chrono::steady_clock::now();
}

void DemoApp::Tick()
{
  const auto now = std::chrono::steady_clock::now();
  const std::chrono::duration<float> frameTime = now - previousFrameTime_;
  previousFrameTime_ = now;

  const float deltaSeconds = std::min(frameTime.count(), 0.05f);
  Update(deltaSeconds);
  renderer_.Render(camera_,
                   obstacleCube_,
                   playerCube_,
                   collisionActive_,
                   selectionController_.IsObstacleSelected(),
                   selectionController_.IsPlayerSelected());
}

void DemoApp::Update(float deltaSeconds)
{
  math::Float3 previousCameraPosition = camera_.GetPosition();
  if (!selectionController_.HasActiveTransform())
  {
    camera_.UpdateFromInput(window_.GetHandle(), deltaSeconds, cameraKeyMap_);
  }

  const math::Float3 cameraPosition = camera_.GetPosition();
  const OBB obstacleCollider = MakeOBB(obstacleCube_);

  math::Float3 resolvedCameraPosition = previousCameraPosition;

  math::Float3 candidateCameraPosition = resolvedCameraPosition;
  candidateCameraPosition.x = cameraPosition.x;
  if (!Intersects(MakeOBB(candidateCameraPosition, math::MakeFloat3(0.0f, 0.0f, 0.0f), kCameraCollisionExtents),
                  obstacleCollider))
  {
    resolvedCameraPosition.x = candidateCameraPosition.x;
  }

  candidateCameraPosition = resolvedCameraPosition;
  candidateCameraPosition.y = cameraPosition.y;
  if (!Intersects(MakeOBB(candidateCameraPosition, math::MakeFloat3(0.0f, 0.0f, 0.0f), kCameraCollisionExtents),
                  obstacleCollider))
  {
    resolvedCameraPosition.y = candidateCameraPosition.y;
  }

  candidateCameraPosition = resolvedCameraPosition;
  candidateCameraPosition.z = cameraPosition.z;
  if (!Intersects(MakeOBB(candidateCameraPosition, math::MakeFloat3(0.0f, 0.0f, 0.0f), kCameraCollisionExtents),
                  obstacleCollider))
  {
    resolvedCameraPosition.z = candidateCameraPosition.z;
  }

  camera_.SetPosition(resolvedCameraPosition);

  if (selectionController_.HasActiveTransform())
  {
    selectionController_.Update(window_.GetHandle(),
                                camera_,
                                window_.GetClientWidth(),
                                window_.GetClientHeight(),
                                playerCube_,
                                obstacleCube_);
  }
  else
  {
    float moveX = 0.0f;
    float moveZ = 0.0f;

    if (input::IsKeyDown(cubeKeyMap_.moveLeft))
    {
      moveX -= cubeKeyMap_.moveSpeed * deltaSeconds;
    }
    if (input::IsKeyDown(cubeKeyMap_.moveRight))
    {
      moveX += cubeKeyMap_.moveSpeed * deltaSeconds;
    }
    if (input::IsKeyDown(cubeKeyMap_.moveForward))
    {
      moveZ += cubeKeyMap_.moveSpeed * deltaSeconds;
    }
    if (input::IsKeyDown(cubeKeyMap_.moveBackward))
    {
      moveZ -= cubeKeyMap_.moveSpeed * deltaSeconds;
    }

    CubeState nextCube = playerCube_;
    nextCube.position.x = std::clamp(nextCube.position.x + moveX, -3.6f, 3.6f);
    if (!Intersects(MakeOBB(nextCube), MakeOBB(obstacleCube_)))
    {
      playerCube_.position.x = nextCube.position.x;
    }

    nextCube = playerCube_;
    nextCube.position.z = std::clamp(nextCube.position.z + moveZ, -3.0f, 3.0f);
    if (!Intersects(MakeOBB(nextCube), MakeOBB(obstacleCube_)))
    {
      playerCube_.position.z = nextCube.position.z;
    }
  }

  collisionActive_ = Intersects(MakeOBB(playerCube_), MakeOBB(obstacleCube_));
}

void DemoApp::HandleAntiAliasingHotkeys(WPARAM key)
{
  switch (key)
  {
  case '1':
    renderer_.SetAntiAliasingMode(AntiAliasingMode::None);
    break;
  case '2':
    renderer_.SetAntiAliasingMode(AntiAliasingMode::FXAA);
    break;
  case '3':
    renderer_.SetAntiAliasingMode(AntiAliasingMode::SMAA);
    break;
  default:
    break;
  }
}

void DemoApp::SetupMenu()
{
  if (mainMenu_)
  {
    return;
  }

  mainMenu_ = CreateMenu();
  renderMenu_ = CreateMenu();

  AppendMenuW(renderMenu_, MF_STRING, kMenuRenderRaster, L"Raster");
  AppendMenuW(renderMenu_, MF_STRING, kMenuRenderRayTrace, L"Ray Tracing (GPU)");
  AppendMenuW(mainMenu_, MF_POPUP, reinterpret_cast<UINT_PTR>(renderMenu_), L"Mode");

  SetMenu(window_.GetHandle(), mainMenu_);
  UpdateRenderMenuChecks();
  DrawMenuBar(window_.GetHandle());
}

void DemoApp::UpdateRenderMenuChecks()
{
  if (!renderMenu_)
  {
    return;
  }

  const RenderMode mode = renderer_.GetRenderMode();
  const UINT selectedCommand = (mode == RenderMode::RayTrace) ? kMenuRenderRayTrace : kMenuRenderRaster;
  CheckMenuRadioItem(renderMenu_, kMenuRenderRaster, kMenuRenderRayTrace, selectedCommand, MF_BYCOMMAND);
}

void DemoApp::UpdateWindowTitle()
{
  const std::wstring title = selectionController_.BuildWindowTitle(kWindowTitle);
  SetWindowTextW(window_.GetHandle(), title.c_str());
}
