#include "GpuRayTracer.hpp"

#include "utils/Math.hpp"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>

using Microsoft::WRL::ComPtr;

namespace
{
struct alignas(16) RayTraceConstants
{
  math::Float4 cameraPosition;
  math::Float4 cameraForward;
  math::Float4 cameraRight;
  math::Float4 cameraUp;
  math::Float4 lightDirection;
  math::Float4 screenInfo;
  math::Float4 obstacleColor;
  math::Float4 obstaclePosition;
  math::Float4 obstacleRotation;
  math::Float4 obstacleExtents;
  math::Float4 playerColor;
  math::Float4 playerPosition;
  math::Float4 playerRotation;
  math::Float4 playerExtents;
  math::Float4 rayTraceSettings;
};

std::string HrToString(HRESULT hr)
{
  char buffer[32] = {};
  std::snprintf(buffer, sizeof(buffer), "0x%08X", static_cast<unsigned>(hr));
  return buffer;
}

void ThrowIfFailed(HRESULT hr, const char* message)
{
  if (FAILED(hr))
  {
    throw std::runtime_error(std::string(message) + " failed with HRESULT " + HrToString(hr));
  }
}

std::filesystem::path GetExecutableDirectory()
{
  wchar_t modulePath[MAX_PATH] = {};
  const DWORD length = GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
  if (length == 0 || length == MAX_PATH)
  {
    throw std::runtime_error("GetModuleFileNameW failed");
  }

  return std::filesystem::path(modulePath).parent_path();
}

std::filesystem::path GetShaderPath(const wchar_t* fileName)
{
  return GetExecutableDirectory() / L"res" / L"shaders" / fileName;
}

math::Float3 NormalizeOrFallback(const math::Float3& value, const math::Float3& fallback)
{
  return math::NormalizeOrFallback(value, fallback);
}
}  // namespace

struct GpuRayTracer::Impl
{
  std::uint32_t width = 1;
  std::uint32_t height = 1;

  ComPtr<ID3D11ComputeShader> computeShader;
  ComPtr<ID3D11Buffer> constantBuffer;
  ComPtr<ID3D11Texture2D> outputTexture;
  ComPtr<ID3D11ShaderResourceView> outputShaderResourceView;
  ComPtr<ID3D11UnorderedAccessView> outputUnorderedAccessView;

  void CreateShader(ID3D11Device* device);
  void CreateConstantBuffer(ID3D11Device* device);
  void CreateOutputTexture(ID3D11Device* device);
  RayTraceConstants BuildConstants(const Camera& camera,
                                   const CubeState& obstacleCube,
                                   const math::Float4& obstacleColor,
                                   const CubeState& playerCube,
                                   const math::Float4& playerColor) const;
};

GpuRayTracer::GpuRayTracer() = default;
GpuRayTracer::~GpuRayTracer() = default;

void GpuRayTracer::Initialize(ID3D11Device* device, std::uint32_t width, std::uint32_t height)
{
  if (!device)
  {
    throw std::runtime_error("GpuRayTracer::Initialize requires a valid device");
  }

  if (!impl_)
  {
    impl_ = std::make_unique<Impl>();
  }

  impl_->width = std::max(width, 1u);
  impl_->height = std::max(height, 1u);

  if (!impl_->computeShader)
  {
    impl_->CreateShader(device);
  }

  if (!impl_->constantBuffer)
  {
    impl_->CreateConstantBuffer(device);
  }

  impl_->CreateOutputTexture(device);
}

void GpuRayTracer::Resize(ID3D11Device* device, std::uint32_t width, std::uint32_t height)
{
  Initialize(device, width, height);
}

void GpuRayTracer::Render(ID3D11DeviceContext* context,
                          const Camera& camera,
                          const CubeState& obstacleCube,
                          const math::Float4& obstacleColor,
                          const CubeState& playerCube,
                          const math::Float4& playerColor)
{
  if (!impl_ || !context || !impl_->outputUnorderedAccessView)
  {
    return;
  }

  const RayTraceConstants constants =
      impl_->BuildConstants(camera, obstacleCube, obstacleColor, playerCube, playerColor);

  D3D11_MAPPED_SUBRESOURCE mapped = {};
  ThrowIfFailed(context->Map(impl_->constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped),
                "Map(ray trace constants)");
  std::memcpy(mapped.pData, &constants, sizeof(constants));
  context->Unmap(impl_->constantBuffer.Get(), 0);

  ID3D11Buffer* constantBuffers[] = {impl_->constantBuffer.Get()};
  context->CSSetConstantBuffers(0, 1, constantBuffers);
  context->CSSetShader(impl_->computeShader.Get(), nullptr, 0);

  ID3D11UnorderedAccessView* unorderedAccessViews[] = {impl_->outputUnorderedAccessView.Get()};
  UINT initialCounts[] = {0u};
  context->CSSetUnorderedAccessViews(0, 1, unorderedAccessViews, initialCounts);

  const UINT groupCountX = (impl_->width + 7u) / 8u;
  const UINT groupCountY = (impl_->height + 7u) / 8u;
  context->Dispatch(groupCountX, groupCountY, 1);

  ID3D11UnorderedAccessView* nullUnorderedAccessViews[] = {nullptr};
  context->CSSetUnorderedAccessViews(0, 1, nullUnorderedAccessViews, initialCounts);
  ID3D11Buffer* nullConstantBuffers[] = {nullptr};
  context->CSSetConstantBuffers(0, 1, nullConstantBuffers);
  context->CSSetShader(nullptr, nullptr, 0);
}

ID3D11ShaderResourceView* GpuRayTracer::GetOutputShaderResourceView() const
{
  return impl_ ? impl_->outputShaderResourceView.Get() : nullptr;
}

std::uint32_t GpuRayTracer::GetWidth() const
{
  return impl_ ? impl_->width : 0u;
}

std::uint32_t GpuRayTracer::GetHeight() const
{
  return impl_ ? impl_->height : 0u;
}

void GpuRayTracer::Impl::CreateShader(ID3D11Device* device)
{
  ComPtr<ID3DBlob> shaderBlob;
  ComPtr<ID3DBlob> errorBlob;
  const auto shaderPath = GetShaderPath(L"raytrace_cs.hlsl");
  const HRESULT hr = D3DCompileFromFile(shaderPath.c_str(),
                                        nullptr,
                                        D3D_COMPILE_STANDARD_FILE_INCLUDE,
                                        "main",
                                        "cs_5_0",
                                        0,
                                        0,
                                        shaderBlob.GetAddressOf(),
                                        errorBlob.GetAddressOf());
  if (FAILED(hr))
  {
    const std::string message =
        errorBlob ? static_cast<const char*>(errorBlob->GetBufferPointer()) : "Compute shader compilation failed";
    throw std::runtime_error(message);
  }

  ThrowIfFailed(device->CreateComputeShader(shaderBlob->GetBufferPointer(),
                                            shaderBlob->GetBufferSize(),
                                            nullptr,
                                            computeShader.GetAddressOf()),
                "CreateComputeShader(raytrace)");
}

void GpuRayTracer::Impl::CreateConstantBuffer(ID3D11Device* device)
{
  D3D11_BUFFER_DESC bufferDesc = {};
  bufferDesc.ByteWidth = sizeof(RayTraceConstants);
  bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
  bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

  ThrowIfFailed(device->CreateBuffer(&bufferDesc, nullptr, constantBuffer.GetAddressOf()),
                "CreateBuffer(raytrace constants)");
}

void GpuRayTracer::Impl::CreateOutputTexture(ID3D11Device* device)
{
  outputShaderResourceView.Reset();
  outputUnorderedAccessView.Reset();
  outputTexture.Reset();

  D3D11_TEXTURE2D_DESC textureDesc = {};
  textureDesc.Width = width;
  textureDesc.Height = height;
  textureDesc.MipLevels = 1;
  textureDesc.ArraySize = 1;
  textureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
  textureDesc.SampleDesc.Count = 1;
  textureDesc.Usage = D3D11_USAGE_DEFAULT;
  textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;

  ThrowIfFailed(device->CreateTexture2D(&textureDesc, nullptr, outputTexture.GetAddressOf()),
                "CreateTexture2D(raytrace output)");
  ThrowIfFailed(device->CreateShaderResourceView(outputTexture.Get(),
                                                 nullptr,
                                                 outputShaderResourceView.GetAddressOf()),
                "CreateShaderResourceView(raytrace output)");
  ThrowIfFailed(device->CreateUnorderedAccessView(outputTexture.Get(),
                                                  nullptr,
                                                  outputUnorderedAccessView.GetAddressOf()),
                "CreateUnorderedAccessView(raytrace output)");
}

RayTraceConstants GpuRayTracer::Impl::BuildConstants(const Camera& camera,
                                                     const CubeState& obstacleCube,
                                                     const math::Float4& obstacleColor,
                                                     const CubeState& playerCube,
                                                     const math::Float4& playerColor) const
{
  const math::Float3 cameraPosition = camera.GetPosition();
  const float yaw = camera.GetYaw();
  const float pitch = camera.GetPitch();
  const float cosPitch = std::cos(pitch);

  const math::Float3 forward =
      NormalizeOrFallback(math::MakeFloat3(std::sin(yaw) * cosPitch, std::sin(pitch), std::cos(yaw) * cosPitch),
                          math::MakeFloat3(0.0f, 0.0f, 1.0f));
  const math::Float3 worldUp = math::MakeFloat3(0.0f, 1.0f, 0.0f);
  const math::Float3 right =
      NormalizeOrFallback(math::StoreFloat3(math::Cross3(math::Load(worldUp), math::Load(forward))),
                          math::MakeFloat3(1.0f, 0.0f, 0.0f));
  const math::Float3 up =
      NormalizeOrFallback(math::StoreFloat3(math::Cross3(math::Load(forward), math::Load(right))), worldUp);

  const float aspect = static_cast<float>(width) / static_cast<float>(std::max(height, 1u));
  const float tanHalfFov = std::tan(math::ToRadians(60.0f) * 0.5f);
  const math::Float3 lightDirection = NormalizeOrFallback(math::MakeFloat3(0.5f, 1.0f, -0.4f),
                                                          math::MakeFloat3(0.0f, 1.0f, 0.0f));

  RayTraceConstants constants = {};
  constants.cameraPosition = math::MakeFloat4(cameraPosition.x, cameraPosition.y, cameraPosition.z, 1.0f);
  constants.cameraForward = math::MakeFloat4(forward.x, forward.y, forward.z, 0.0f);
  constants.cameraRight = math::MakeFloat4(right.x, right.y, right.z, 0.0f);
  constants.cameraUp = math::MakeFloat4(up.x, up.y, up.z, 0.0f);
  constants.lightDirection = math::MakeFloat4(lightDirection.x, lightDirection.y, lightDirection.z, 0.0f);
  constants.screenInfo = math::MakeFloat4(static_cast<float>(width),
                                          static_cast<float>(height),
                                          tanHalfFov,
                                          aspect);
  constants.obstacleColor = obstacleColor;
  constants.obstaclePosition =
      math::MakeFloat4(obstacleCube.position.x, obstacleCube.position.y, obstacleCube.position.z, 1.0f);
  constants.obstacleRotation =
      math::MakeFloat4(obstacleCube.rotation.x, obstacleCube.rotation.y, obstacleCube.rotation.z, 0.0f);
  constants.obstacleExtents =
      math::MakeFloat4(obstacleCube.extents.x, obstacleCube.extents.y, obstacleCube.extents.z, 1.0f);
  constants.playerColor = playerColor;
  constants.playerPosition =
      math::MakeFloat4(playerCube.position.x, playerCube.position.y, playerCube.position.z, 1.0f);
  constants.playerRotation =
      math::MakeFloat4(playerCube.rotation.x, playerCube.rotation.y, playerCube.rotation.z, 0.0f);
  constants.playerExtents =
      math::MakeFloat4(playerCube.extents.x, playerCube.extents.y, playerCube.extents.z, 0.0f);
  constants.rayTraceSettings = math::MakeFloat4(1.0e-3f, 64.0f, 0.5f, 0.2f);
  return constants;
}
