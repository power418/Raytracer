#include "DemoApp.hpp"

#include "Collision.hpp"
#include "utils/Input.hpp"
#include "utils/Math.hpp"

#include <algorithm>

namespace
{
constexpr wchar_t kWindowTitle[] = L"DirectX 11 Demo Application";
constexpr math::Float3 kCameraCollisionExtents = {0.35f, 0.65f, 0.35f};
constexpr UINT kMenuRenderRaster = 40001;
constexpr UINT kMenuRenderRayTrace = 40002;
}

DemoApp::DemoApp(HINSTANCE instance)
  : instance_(instance),
    window_(instance_, *this, kWindowTitle, 1280, 720),
    cameraKeyMap_(input::MakeUnrealStyleCameraKeyMap()),
    cubeKeyMap_(input::MakeDemoCubeKeyMap())
{
  playerCube_.position = math::MakeFloat3(-2.2f, 0.0f, 0.0f);
  obstacleCube_.position = math::MakeFloat3(0.0f, 0.0f, 0.0f);
}

DemoApp::~DemoApp() = default;

int DemoApp::Run(int showCommand)
{
  window_.Show(showCommand);
  renderer_.Initialize(window_.GetHandle(), window_.GetClientWidth(), window_.GetClientHeight());
  SetupMenu();
  ResetTimer();

  while (window_.ProcessMessages())
  {
    Tick();
  }

  return 0;
}

void DemoApp::OnResize(std::uint32_t width, std::uint32_t height)
{
  renderer_.Resize(width, height);
}

void DemoApp::OnKeyDown(WPARAM key)
{
  if (key == VK_ESCAPE)
  {
    DestroyWindow(window_.GetHandle());
    return;
  }

  HandleAntiAliasingHotkeys(key);
}

void DemoApp::OnCommand(WPARAM wParam, LPARAM)
{
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
  renderer_.Render(camera_, obstacleCube_, playerCube_, collisionActive_);
}

void DemoApp::Update(float deltaSeconds)
{
  const math::Float3 previousCameraPosition = camera_.GetPosition();
  camera_.UpdateFromInput(window_.GetHandle(), deltaSeconds, cameraKeyMap_);

  const math::Float3 cameraPosition = camera_.GetPosition();
  const OBB obstacleCollider = MakeOBB(obstacleCube_);

  math::Float3 resolvedCameraPosition = previousCameraPosition;

  math::Float3 candidateCameraPosition = resolvedCameraPosition;
  candidateCameraPosition.x = cameraPosition.x;
  if (!Intersects(MakeOBB(candidateCameraPosition, math::MakeFloat3(0.0f, 0.0f, 0.0f), kCameraCollisionExtents), obstacleCollider))
  {
    resolvedCameraPosition.x = candidateCameraPosition.x;
  }

  candidateCameraPosition = resolvedCameraPosition;
  candidateCameraPosition.y = cameraPosition.y;
  if (!Intersects(MakeOBB(candidateCameraPosition, math::MakeFloat3(0.0f, 0.0f, 0.0f), kCameraCollisionExtents), obstacleCollider))
  {
    resolvedCameraPosition.y = candidateCameraPosition.y;
  }

  candidateCameraPosition = resolvedCameraPosition;
  candidateCameraPosition.z = cameraPosition.z;
  if (!Intersects(MakeOBB(candidateCameraPosition, math::MakeFloat3(0.0f, 0.0f, 0.0f), kCameraCollisionExtents), obstacleCollider))
  {
    resolvedCameraPosition.z = candidateCameraPosition.z;
  }

  camera_.SetPosition(resolvedCameraPosition);

  obstacleCube_.rotation.y += deltaSeconds * math::kHalfPi * 0.75f;
  if (obstacleCube_.rotation.y > math::kTwoPi)
  {
    obstacleCube_.rotation.y -= math::kTwoPi;
  }

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

  collisionActive_ = Intersects(MakeOBB(playerCube_), obstacleCollider);
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
