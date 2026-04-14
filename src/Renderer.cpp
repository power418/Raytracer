#include "../include/Renderer.hpp"
#include "../include/EndlessGrid.hpp"
#include "../include/GraphicsAdapter.hpp"
#include "../include/GpuRayTracer.hpp"
#include "../utils/Math.hpp"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <wrl/client.h>

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")

using Microsoft::WRL::ComPtr;

namespace {
struct Vertex {
  math::Float3 position;
  math::Float3 normal;
};

struct alignas(16) ObjectConstants {
  math::Matrix worldViewProjection;
  math::Matrix world;
  math::Float4 color;
  math::Float4 lightDirection;
};

struct alignas(16) GridConstants {
  math::Matrix worldViewProjection;
  math::Matrix world;
  math::Float4 minorColor;
  math::Float4 majorColor;
  math::Float4 gridParams;
  math::Float4 cameraWorldPosition;
};

struct FullscreenVertex {
  math::Float3 position;
  math::Float2 uv;
};

struct alignas(16) PostProcessConstants {
  math::Float2 texelSize;
  math::Float2 screenSize;
  math::Float4 parameters;
};

std::string HrToString(HRESULT hr) {
  char buffer[32] = {};
  std::snprintf(buffer, sizeof(buffer), "0x%08X", static_cast<unsigned>(hr));
  return buffer;
}

void ThrowIfFailed(HRESULT hr, const char *message) {
  if (FAILED(hr)) {
    throw std::runtime_error(std::string(message) + " failed with HRESULT " +
                             HrToString(hr));
  }
}

std::filesystem::path GetExecutableDirectory() {
  wchar_t modulePath[MAX_PATH] = {};
  const DWORD length = GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
  if (length == 0 || length == MAX_PATH) {
    throw std::runtime_error("GetModuleFileNameW failed");
  }

  return std::filesystem::path(modulePath).parent_path();
}

std::filesystem::path GetShaderPath(const wchar_t *fileName) {
  return GetExecutableDirectory() / L"res" / L"shaders" / fileName;
}
} // namespace

struct Renderer::Impl {
  std::uint32_t width = 1280;
  std::uint32_t height = 720;
  std::uint32_t rayTraceWidth = 640;
  std::uint32_t rayTraceHeight = 360;
  UINT indexCount = 0;
  D3D11_VIEWPORT viewport = {};
  gfx::GraphicsAdapterInfo adapterInfo = {};
  AntiAliasingMode antiAliasingMode = AntiAliasingMode::FXAA;
  RenderMode renderMode = RenderMode::Raster;

  ComPtr<ID3D11Device> device;
  ComPtr<ID3D11DeviceContext> context;
  ComPtr<IDXGISwapChain> swapChain;
  ComPtr<ID3D11RenderTargetView> backBufferRenderTargetView;
  ComPtr<ID3D11Texture2D> depthBuffer;
  ComPtr<ID3D11DepthStencilView> depthStencilView;

  ComPtr<ID3D11Texture2D> sceneColorTexture;
  ComPtr<ID3D11RenderTargetView> sceneColorRenderTargetView;
  ComPtr<ID3D11ShaderResourceView> sceneColorShaderResourceView;

  GpuRayTracer gpuRayTracer;

  EndlessGrid endlessGrid;

  ComPtr<ID3D11Texture2D> smaaEdgeTexture;
  ComPtr<ID3D11RenderTargetView> smaaEdgeRenderTargetView;
  ComPtr<ID3D11ShaderResourceView> smaaEdgeShaderResourceView;

  ComPtr<ID3D11VertexShader> gridVertexShader;
  ComPtr<ID3D11PixelShader> gridPixelShader;
  ComPtr<ID3D11InputLayout> gridInputLayout;
  ComPtr<ID3D11Buffer> gridVertexBuffer;
  ComPtr<ID3D11Buffer> gridConstantBuffer;

  ComPtr<ID3D11VertexShader> vertexShader;
  ComPtr<ID3D11PixelShader> pixelShader;
  ComPtr<ID3D11InputLayout> inputLayout;
  ComPtr<ID3D11Buffer> vertexBuffer;
  ComPtr<ID3D11Buffer> indexBuffer;
  ComPtr<ID3D11Buffer> objectConstantBuffer;

  ComPtr<ID3D11VertexShader> fullscreenVertexShader;
  ComPtr<ID3D11PixelShader> copyPixelShader;
  ComPtr<ID3D11PixelShader> fxaaPixelShader;
  ComPtr<ID3D11PixelShader> smaaEdgePixelShader;
  ComPtr<ID3D11PixelShader> smaaBlendPixelShader;
  ComPtr<ID3D11InputLayout> fullscreenInputLayout;
  ComPtr<ID3D11Buffer> fullscreenVertexBuffer;
  ComPtr<ID3D11Buffer> postProcessConstantBuffer;

  ComPtr<ID3D11RasterizerState> rasterizerState;
  ComPtr<ID3D11RasterizerState> gridRasterizerState;
  ComPtr<ID3D11RasterizerState> selectionRasterizerState;
  ComPtr<ID3D11DepthStencilState> depthStencilState;
  ComPtr<ID3D11DepthStencilState> disabledDepthStencilState;
  ComPtr<ID3D11BlendState> blendState;
  ComPtr<ID3D11SamplerState> linearClampSamplerState;

  void CreateRenderTargets();
  void CreateShaders();
  void CreateCubeBuffers();
  void CreateGridBuffers();
  void CreateFullscreenQuad();
  void CreateConstantBuffers();
  void CreatePipelineStates();
  void DrawGrid(const Camera &camera);
  void DrawCube(const math::Matrix &viewMatrix, const CubeState &cube,
                const math::Float4 &color);
  void DrawSelectionOverlay(const Camera &camera,
                            const CubeState &obstacleCube,
                            const CubeState &playerCube,
                            bool obstacleSelected,
                            bool playerSelected);
  void DrawSelectionCube(const math::Matrix &viewMatrix, const CubeState &cube);
  void BeginScenePass();
  void ResolveToBackBuffer(ID3D11ShaderResourceView *source,
                           std::uint32_t sourceWidth,
                           std::uint32_t sourceHeight);
  void
  ExecuteFullscreenPass(ID3D11PixelShader *pixelShader,
                        ID3D11ShaderResourceView *primarySource,
                        ID3D11ShaderResourceView *secondarySource = nullptr);
  void UpdatePostProcessConstants(std::uint32_t sourceWidth,
                                  std::uint32_t sourceHeight,
                                  float edgeThreshold, float blendStrength);
  void UpdateRayTraceTexture(const Camera &camera, const CubeState &obstacleCube,
                             const CubeState &playerCube,
                             bool collisionActive);
};

Renderer::Renderer() = default;
Renderer::~Renderer() = default;

void Renderer::Initialize(HWND windowHandle, std::uint32_t width,
                          std::uint32_t height) {
  std::printf("[Renderer] Initializing (HWND=%p, %ux%u)...\n",
              static_cast<void*>(windowHandle), width, height);
  impl_ = std::make_unique<Impl>();
  impl_->width = width;
  impl_->height = height;

  UINT deviceFlags = 0;
#if defined(_DEBUG)
  deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
  std::printf("[Renderer] Debug device flags enabled\n");
#endif

  std::printf("[Renderer] Creating D3D11 device and swap chain...\n");
  const HRESULT hr = gfx::CreateHighPerformanceDeviceAndSwapChain(
      windowHandle, width, height, deviceFlags, impl_->swapChain.GetAddressOf(),
      impl_->device.GetAddressOf(), impl_->context.GetAddressOf(),
      &impl_->adapterInfo);
  ThrowIfFailed(hr, "CreateHighPerformanceDeviceAndSwapChain");
  std::printf("[Renderer] D3D11 device created successfully\n");

  std::printf("[Renderer] Creating render targets...\n");
  impl_->CreateRenderTargets();
  std::printf("[Renderer] Render targets created\n");

  std::printf("[Renderer] Compiling shaders...\n");
  impl_->CreateShaders();
  std::printf("[Renderer] Shaders compiled successfully\n");

  std::printf("[Renderer] Creating geometry buffers...\n");
  impl_->CreateCubeBuffers();
  impl_->CreateGridBuffers();
  impl_->CreateFullscreenQuad();
  std::printf("[Renderer] Geometry buffers created\n");

  std::printf("[Renderer] Creating constant buffers...\n");
  impl_->CreateConstantBuffers();
  std::printf("[Renderer] Constant buffers created\n");

  std::printf("[Renderer] Creating pipeline states...\n");
  impl_->CreatePipelineStates();
  std::printf("[Renderer] Pipeline states created\n");

  std::printf("[Renderer] Initialization complete\n");
}

void Renderer::Resize(std::uint32_t width, std::uint32_t height) {
  if (!impl_ || !impl_->device || width == 0 || height == 0) {
    return;
  }

  std::printf("[Renderer] Resizing to %ux%u...\n", width, height);
  impl_->width = width;
  impl_->height = height;

  impl_->context->OMSetRenderTargets(0, nullptr, nullptr);
  impl_->backBufferRenderTargetView.Reset();
  impl_->sceneColorRenderTargetView.Reset();
  impl_->sceneColorShaderResourceView.Reset();
  impl_->sceneColorTexture.Reset();
  impl_->smaaEdgeRenderTargetView.Reset();
  impl_->smaaEdgeShaderResourceView.Reset();
  impl_->smaaEdgeTexture.Reset();
  impl_->depthStencilView.Reset();
  impl_->depthBuffer.Reset();

  ThrowIfFailed(
      impl_->swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0),
      "IDXGISwapChain::ResizeBuffers");

  impl_->CreateRenderTargets();
  std::printf("[Renderer] Resize complete\n");
}

void Renderer::Render(const Camera &camera, const CubeState &obstacleCube,
                      const CubeState &playerCube, bool collisionActive,
                      bool obstacleSelected, bool playerSelected) {
  if (!impl_) {
    return;
  }

  const math::Matrix viewMatrix = camera.GetViewMatrix();

  ID3D11ShaderResourceView *source = impl_->sceneColorShaderResourceView.Get();
  std::uint32_t sourceWidth = impl_->width;
  std::uint32_t sourceHeight = impl_->height;

  if (impl_->renderMode == RenderMode::Raster) {
    impl_->BeginScenePass();
    impl_->DrawGrid(camera);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    ID3D11Buffer *vertexBuffers[] = {impl_->vertexBuffer.Get()};
    impl_->context->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);
    impl_->context->IASetIndexBuffer(impl_->indexBuffer.Get(),
                                     DXGI_FORMAT_R16_UINT, 0);
    impl_->context->IASetInputLayout(impl_->inputLayout.Get());
    impl_->context->IASetPrimitiveTopology(
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    impl_->context->VSSetShader(impl_->vertexShader.Get(), nullptr, 0);
    impl_->context->PSSetShader(impl_->pixelShader.Get(), nullptr, 0);

    ID3D11Buffer *constantBuffers[] = {impl_->objectConstantBuffer.Get()};
    impl_->context->VSSetConstantBuffers(0, 1, constantBuffers);
    impl_->context->PSSetConstantBuffers(0, 1, constantBuffers);

    impl_->DrawCube(viewMatrix, obstacleCube,
                    collisionActive
                        ? math::MakeFloat4(0.90f, 0.28f, 0.25f, 1.0f)
                        : math::MakeFloat4(0.20f, 0.71f, 0.93f, 1.0f));
    impl_->DrawCube(viewMatrix, playerCube,
                    collisionActive
                        ? math::MakeFloat4(0.99f, 0.83f, 0.20f, 1.0f)
                        : math::MakeFloat4(0.33f, 0.91f, 0.48f, 1.0f));
  } else {
    impl_->UpdateRayTraceTexture(camera, obstacleCube, playerCube,
                                 collisionActive);
    source = impl_->gpuRayTracer.GetOutputShaderResourceView();
    sourceWidth = impl_->gpuRayTracer.GetWidth();
    sourceHeight = impl_->gpuRayTracer.GetHeight();
  }

  impl_->ResolveToBackBuffer(source, sourceWidth, sourceHeight);
  impl_->DrawSelectionOverlay(camera, obstacleCube, playerCube, obstacleSelected,
                              playerSelected);
  impl_->swapChain->Present(1, 0);
}

void Renderer::SetAntiAliasingMode(AntiAliasingMode mode) {
  if (impl_) {
    impl_->antiAliasingMode = mode;
  }
}

AntiAliasingMode Renderer::GetAntiAliasingMode() const {
  return impl_ ? impl_->antiAliasingMode : AntiAliasingMode::None;
}

void Renderer::SetRenderMode(RenderMode mode) {
  if (impl_) {
    impl_->renderMode = mode;
  }
}

RenderMode Renderer::GetRenderMode() const {
  return impl_ ? impl_->renderMode : RenderMode::Raster;
}

void Renderer::Impl::CreateRenderTargets() {
  ComPtr<ID3D11Texture2D> backBuffer;
  ThrowIfFailed(
      swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf())),
      "IDXGISwapChain::GetBuffer");
  ThrowIfFailed(
      device->CreateRenderTargetView(backBuffer.Get(), nullptr,
                                     backBufferRenderTargetView.GetAddressOf()),
      "CreateRenderTargetView");

  D3D11_TEXTURE2D_DESC colorDesc = {};
  colorDesc.Width = width;
  colorDesc.Height = height;
  colorDesc.MipLevels = 1;
  colorDesc.ArraySize = 1;
  colorDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  colorDesc.SampleDesc.Count = 1;
  colorDesc.Usage = D3D11_USAGE_DEFAULT;
  colorDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

  ThrowIfFailed(device->CreateTexture2D(&colorDesc, nullptr,
                                        sceneColorTexture.GetAddressOf()),
                "CreateTexture2D(scene)");
  ThrowIfFailed(
      device->CreateRenderTargetView(sceneColorTexture.Get(), nullptr,
                                     sceneColorRenderTargetView.GetAddressOf()),
      "CreateRenderTargetView(scene)");
  ThrowIfFailed(device->CreateShaderResourceView(
                    sceneColorTexture.Get(), nullptr,
                    sceneColorShaderResourceView.GetAddressOf()),
                "CreateShaderResourceView(scene)");

  rayTraceWidth = std::max(1u, width / 2u);
  rayTraceHeight = std::max(1u, height / 2u);
  gpuRayTracer.Initialize(device.Get(), rayTraceWidth, rayTraceHeight);

  ThrowIfFailed(device->CreateTexture2D(&colorDesc, nullptr,
                                        smaaEdgeTexture.GetAddressOf()),
                "CreateTexture2D(smaa edges)");
  ThrowIfFailed(
      device->CreateRenderTargetView(smaaEdgeTexture.Get(), nullptr,
                                     smaaEdgeRenderTargetView.GetAddressOf()),
      "CreateRenderTargetView(smaa edges)");
  ThrowIfFailed(device->CreateShaderResourceView(
                    smaaEdgeTexture.Get(), nullptr,
                    smaaEdgeShaderResourceView.GetAddressOf()),
                "CreateShaderResourceView(smaa edges)");

  D3D11_TEXTURE2D_DESC depthDesc = {};
  depthDesc.Width = width;
  depthDesc.Height = height;
  depthDesc.MipLevels = 1;
  depthDesc.ArraySize = 1;
  depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  depthDesc.SampleDesc.Count = 1;
  depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

  ThrowIfFailed(
      device->CreateTexture2D(&depthDesc, nullptr, depthBuffer.GetAddressOf()),
      "CreateTexture2D(depth)");
  ThrowIfFailed(device->CreateDepthStencilView(depthBuffer.Get(), nullptr,
                                               depthStencilView.GetAddressOf()),
                "CreateDepthStencilView");

  viewport.TopLeftX = 0.0f;
  viewport.TopLeftY = 0.0f;
  viewport.Width = static_cast<float>(width);
  viewport.Height = static_cast<float>(height);
  viewport.MinDepth = 0.0f;
  viewport.MaxDepth = 1.0f;
}

void Renderer::Impl::CreateShaders() {
  auto compileShader = [&](const wchar_t *fileName, const char *entryPoint,
                           const char *profile,
                           ID3DBlob **shaderBlob) -> ComPtr<ID3DBlob> {
    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> errorBlob;
    const auto shaderPath = GetShaderPath(fileName);
    std::printf("[Renderer]   Compiling shader: %ls (%s)\n", fileName, profile);
    const HRESULT hr = D3DCompileFromFile(
        shaderPath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entryPoint, profile, 0, 0, blob.GetAddressOf(),
        errorBlob.GetAddressOf());
    if (FAILED(hr)) {
      const std::string message =
          errorBlob ? static_cast<const char *>(errorBlob->GetBufferPointer())
                    : "Shader compilation failed";
      std::fprintf(stderr, "[Renderer]   ERROR: Shader %ls failed: %s\n", fileName, message.c_str());
      throw std::runtime_error(message);
    }
    std::printf("[Renderer]   Shader %ls compiled OK (size=%zu bytes)\n", fileName, blob->GetBufferSize());
    if (shaderBlob) {
      *shaderBlob = blob.Get();
      if (*shaderBlob) {
        (*shaderBlob)->AddRef();
      }
    }
    return blob;
  };

  ComPtr<ID3DBlob> sceneVertexBlob;
  const auto basicVertexBlob = compileShader(L"basic_vs.hlsl", "main", "vs_4_0",
                                             sceneVertexBlob.GetAddressOf());
  const auto basicPixelBlob =
      compileShader(L"basic_ps.hlsl", "main", "ps_4_0", nullptr);
  const auto gridVertexBlob =
      compileShader(L"endless_grid_vs.hlsl", "main", "vs_4_0", nullptr);
  const auto gridPixelBlob =
      compileShader(L"endless_grid_ps.hlsl", "main", "ps_4_0", nullptr);
  const auto fullscreenVertexBlob =
      compileShader(L"fullscreen_vs.hlsl", "main", "vs_4_0", nullptr);
  const auto copyPixelBlob =
      compileShader(L"copy_ps.hlsl", "main", "ps_4_0", nullptr);
  const auto fxaaPixelBlob =
      compileShader(L"fxaa_ps.hlsl", "main", "ps_4_0", nullptr);
  const auto smaaEdgePixelBlob =
      compileShader(L"smaa_edge_ps.hlsl", "main", "ps_4_0", nullptr);
  const auto smaaBlendPixelBlob =
      compileShader(L"smaa_blend_ps.hlsl", "main", "ps_4_0", nullptr);

  ThrowIfFailed(device->CreateVertexShader(basicVertexBlob->GetBufferPointer(),
                                           basicVertexBlob->GetBufferSize(),
                                           nullptr,
                                           vertexShader.GetAddressOf()),
                "CreateVertexShader(scene)");
  ThrowIfFailed(device->CreatePixelShader(basicPixelBlob->GetBufferPointer(),
                                          basicPixelBlob->GetBufferSize(),
                                          nullptr, pixelShader.GetAddressOf()),
                "CreatePixelShader(scene)");
  ThrowIfFailed(device->CreateVertexShader(gridVertexBlob->GetBufferPointer(),
                                           gridVertexBlob->GetBufferSize(),
                                           nullptr,
                                           gridVertexShader.GetAddressOf()),
                "CreateVertexShader(grid)");
  ThrowIfFailed(device->CreatePixelShader(gridPixelBlob->GetBufferPointer(),
                                          gridPixelBlob->GetBufferSize(),
                                          nullptr,
                                          gridPixelShader.GetAddressOf()),
                "CreatePixelShader(grid)");

  ThrowIfFailed(
      device->CreateVertexShader(fullscreenVertexBlob->GetBufferPointer(),
                                 fullscreenVertexBlob->GetBufferSize(), nullptr,
                                 fullscreenVertexShader.GetAddressOf()),
      "CreateVertexShader(fullscreen)");
  ThrowIfFailed(device->CreatePixelShader(copyPixelBlob->GetBufferPointer(),
                                          copyPixelBlob->GetBufferSize(),
                                          nullptr,
                                          copyPixelShader.GetAddressOf()),
                "CreatePixelShader(copy)");
  ThrowIfFailed(device->CreatePixelShader(fxaaPixelBlob->GetBufferPointer(),
                                          fxaaPixelBlob->GetBufferSize(),
                                          nullptr,
                                          fxaaPixelShader.GetAddressOf()),
                "CreatePixelShader(fxaa)");
  ThrowIfFailed(device->CreatePixelShader(smaaEdgePixelBlob->GetBufferPointer(),
                                          smaaEdgePixelBlob->GetBufferSize(),
                                          nullptr,
                                          smaaEdgePixelShader.GetAddressOf()),
                "CreatePixelShader(smaa edge)");
  ThrowIfFailed(
      device->CreatePixelShader(smaaBlendPixelBlob->GetBufferPointer(),
                                smaaBlendPixelBlob->GetBufferSize(), nullptr,
                                smaaBlendPixelShader.GetAddressOf()),
      "CreatePixelShader(smaa blend)");

  const D3D11_INPUT_ELEMENT_DESC sceneInputLayoutDesc[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
       static_cast<UINT>(offsetof(Vertex, position)),
       D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
       static_cast<UINT>(offsetof(Vertex, normal)), D3D11_INPUT_PER_VERTEX_DATA,
       0},
  };

  ThrowIfFailed(device->CreateInputLayout(
                    sceneInputLayoutDesc,
                    static_cast<UINT>(std::size(sceneInputLayoutDesc)),
                    basicVertexBlob->GetBufferPointer(),
                    basicVertexBlob->GetBufferSize(),
                    inputLayout.GetAddressOf()),
                "CreateInputLayout(scene)");

  const D3D11_INPUT_ELEMENT_DESC gridInputLayoutDesc[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
       D3D11_INPUT_PER_VERTEX_DATA, 0},
  };

  ThrowIfFailed(device->CreateInputLayout(
                    gridInputLayoutDesc,
                    static_cast<UINT>(std::size(gridInputLayoutDesc)),
                    gridVertexBlob->GetBufferPointer(),
                    gridVertexBlob->GetBufferSize(),
                    gridInputLayout.GetAddressOf()),
                "CreateInputLayout(grid)");

  const D3D11_INPUT_ELEMENT_DESC fullscreenInputLayoutDesc[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
       static_cast<UINT>(offsetof(FullscreenVertex, position)),
       D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
       static_cast<UINT>(offsetof(FullscreenVertex, uv)),
       D3D11_INPUT_PER_VERTEX_DATA, 0},
  };

  ThrowIfFailed(device->CreateInputLayout(
                    fullscreenInputLayoutDesc,
                    static_cast<UINT>(std::size(fullscreenInputLayoutDesc)),
                    fullscreenVertexBlob->GetBufferPointer(),
                    fullscreenVertexBlob->GetBufferSize(),
                    fullscreenInputLayout.GetAddressOf()),
                "CreateInputLayout(fullscreen)");
}

void Renderer::Impl::CreateCubeBuffers() {
  const std::vector<Vertex> vertices = {
      {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}},
      {{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}},
      {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}},
      {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}},
      {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
      {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
      {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
      {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
      {{-0.5f, -0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}},
      {{-0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}},
      {{-0.5f, 0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}},
      {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}},
      {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
      {{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
      {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}},
      {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}},
      {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
      {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
      {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
      {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
      {{-0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}},
      {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}},
      {{0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}},
      {{0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}},
  };

  const std::vector<std::uint16_t> indices = {
      0,  1,  2,  0,  2,  3,  4,  5,  6,  4,  6,  7,  8,  9,  10, 8,  10, 11,
      12, 13, 14, 12, 14, 15, 16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23,
  };

  indexCount = static_cast<UINT>(indices.size());

  D3D11_BUFFER_DESC vertexBufferDesc = {};
  vertexBufferDesc.ByteWidth =
      static_cast<UINT>(vertices.size() * sizeof(Vertex));
  vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
  vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

  D3D11_SUBRESOURCE_DATA vertexData = {};
  vertexData.pSysMem = vertices.data();
  ThrowIfFailed(device->CreateBuffer(&vertexBufferDesc, &vertexData,
                                     vertexBuffer.GetAddressOf()),
                "CreateBuffer(vertex)");

  D3D11_BUFFER_DESC indexBufferDesc = {};
  indexBufferDesc.ByteWidth =
      static_cast<UINT>(indices.size() * sizeof(std::uint16_t));
  indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
  indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

  D3D11_SUBRESOURCE_DATA indexData = {};
  indexData.pSysMem = indices.data();
  ThrowIfFailed(device->CreateBuffer(&indexBufferDesc, &indexData,
                                     indexBuffer.GetAddressOf()),
                "CreateBuffer(index)");
}

void Renderer::Impl::CreateGridBuffers() {
  const std::vector<math::Float3> vertices = {
      {-1.0f, 0.0f, -1.0f},
      {-1.0f, 0.0f, 1.0f},
      {1.0f, 0.0f, -1.0f},
      {1.0f, 0.0f, 1.0f},
  };

  D3D11_BUFFER_DESC bufferDesc = {};
  bufferDesc.ByteWidth =
      static_cast<UINT>(vertices.size() * sizeof(math::Float3));
  bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
  bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

  D3D11_SUBRESOURCE_DATA data = {};
  data.pSysMem = vertices.data();

  ThrowIfFailed(device->CreateBuffer(&bufferDesc, &data,
                                     gridVertexBuffer.GetAddressOf()),
                "CreateBuffer(grid)");
}

void Renderer::Impl::CreateFullscreenQuad() {
  const std::vector<FullscreenVertex> vertices = {
      {{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
      {{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
      {{1.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
      {{1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
  };

  D3D11_BUFFER_DESC bufferDesc = {};
  bufferDesc.ByteWidth =
      static_cast<UINT>(vertices.size() * sizeof(FullscreenVertex));
  bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
  bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

  D3D11_SUBRESOURCE_DATA data = {};
  data.pSysMem = vertices.data();

  ThrowIfFailed(device->CreateBuffer(&bufferDesc, &data,
                                     fullscreenVertexBuffer.GetAddressOf()),
                "CreateBuffer(fullscreen)");
}

void Renderer::Impl::CreateConstantBuffers() {
  D3D11_BUFFER_DESC objectBufferDesc = {};
  objectBufferDesc.ByteWidth = sizeof(ObjectConstants);
  objectBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
  objectBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  objectBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  ThrowIfFailed(device->CreateBuffer(&objectBufferDesc, nullptr,
                                     objectConstantBuffer.GetAddressOf()),
                "CreateBuffer(object constants)");

  D3D11_BUFFER_DESC gridBufferDesc = {};
  gridBufferDesc.ByteWidth = sizeof(GridConstants);
  gridBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
  gridBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  gridBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  ThrowIfFailed(device->CreateBuffer(&gridBufferDesc, nullptr,
                                     gridConstantBuffer.GetAddressOf()),
                "CreateBuffer(grid constants)");

  D3D11_BUFFER_DESC postBufferDesc = {};
  postBufferDesc.ByteWidth = sizeof(PostProcessConstants);
  postBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
  postBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  postBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  ThrowIfFailed(device->CreateBuffer(&postBufferDesc, nullptr,
                                     postProcessConstantBuffer.GetAddressOf()),
                "CreateBuffer(postprocess constants)");
}

void Renderer::Impl::CreatePipelineStates() {
  D3D11_RASTERIZER_DESC rasterizerDesc = {};
  rasterizerDesc.FillMode = D3D11_FILL_SOLID;
  rasterizerDesc.CullMode = D3D11_CULL_BACK;
  rasterizerDesc.DepthClipEnable = TRUE;
  ThrowIfFailed(device->CreateRasterizerState(&rasterizerDesc,
                                              rasterizerState.GetAddressOf()),
                "CreateRasterizerState");

  D3D11_RASTERIZER_DESC gridRasterizerDesc = rasterizerDesc;
  gridRasterizerDesc.CullMode = D3D11_CULL_NONE;
  ThrowIfFailed(device->CreateRasterizerState(
                    &gridRasterizerDesc, gridRasterizerState.GetAddressOf()),
                "CreateRasterizerState(grid)");

  D3D11_RASTERIZER_DESC selectionRasterizerDesc = rasterizerDesc;
  selectionRasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
  selectionRasterizerDesc.CullMode = D3D11_CULL_NONE;
  ThrowIfFailed(device->CreateRasterizerState(
                    &selectionRasterizerDesc,
                    selectionRasterizerState.GetAddressOf()),
                "CreateRasterizerState(selection)");

  D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
  depthStencilDesc.DepthEnable = TRUE;
  depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
  depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
  ThrowIfFailed(device->CreateDepthStencilState(
                    &depthStencilDesc, depthStencilState.GetAddressOf()),
                "CreateDepthStencilState");

  D3D11_DEPTH_STENCIL_DESC disabledDepthDesc = {};
  disabledDepthDesc.DepthEnable = FALSE;
  disabledDepthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
  disabledDepthDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
  ThrowIfFailed(
      device->CreateDepthStencilState(&disabledDepthDesc,
                                      disabledDepthStencilState.GetAddressOf()),
      "CreateDepthStencilState(disabled)");

  D3D11_BLEND_DESC blendDesc = {};
  blendDesc.RenderTarget[0].BlendEnable = FALSE;
  blendDesc.RenderTarget[0].RenderTargetWriteMask =
      D3D11_COLOR_WRITE_ENABLE_ALL;
  ThrowIfFailed(device->CreateBlendState(&blendDesc, blendState.GetAddressOf()),
                "CreateBlendState");

  D3D11_SAMPLER_DESC samplerDesc = {};
  samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
  samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
  samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
  samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
  ThrowIfFailed(device->CreateSamplerState(
                    &samplerDesc, linearClampSamplerState.GetAddressOf()),
                "CreateSamplerState");
}

void Renderer::Impl::DrawGrid(const Camera &camera) {
  const math::Matrix viewMatrix = camera.GetViewMatrix();
  const math::Matrix projection = math::PerspectiveFovLH(
      math::ToRadians(60.0f),
      static_cast<float>(width) / static_cast<float>(std::max(height, 1u)),
      0.1f, 1000.0f);

  const EndlessGrid::Settings &settings = endlessGrid.GetSettings();
  const math::Matrix world = endlessGrid.BuildWorldMatrix(camera.GetPosition());

  GridConstants constants = {};
  constants.worldViewProjection =
      math::Transpose(world * viewMatrix * projection);
  constants.world = math::Transpose(world);
  constants.minorColor = settings.minorColor;
  constants.majorColor = settings.majorColor;
  constants.gridParams = math::MakeFloat4(
      settings.cellSize, settings.majorScale, settings.fadeDistance, 0.0f);
  const math::Float3 cameraPosition = camera.GetPosition();
  constants.cameraWorldPosition =
      math::MakeFloat4(cameraPosition.x, cameraPosition.y, cameraPosition.z, 1.0f);

  D3D11_MAPPED_SUBRESOURCE mapped = {};
  ThrowIfFailed(context->Map(gridConstantBuffer.Get(), 0,
                             D3D11_MAP_WRITE_DISCARD, 0, &mapped),
                "Map(grid constants)");
  std::memcpy(mapped.pData, &constants, sizeof(constants));
  context->Unmap(gridConstantBuffer.Get(), 0);

  UINT stride = sizeof(math::Float3);
  UINT offset = 0;
  ID3D11Buffer *vertexBuffers[] = {gridVertexBuffer.Get()};
  context->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);
  context->IASetInputLayout(gridInputLayout.Get());
  context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
  context->RSSetState(gridRasterizerState.Get());
  context->VSSetShader(gridVertexShader.Get(), nullptr, 0);
  context->PSSetShader(gridPixelShader.Get(), nullptr, 0);

  ID3D11Buffer *buffers[] = {gridConstantBuffer.Get()};
  context->VSSetConstantBuffers(0, 1, buffers);
  context->PSSetConstantBuffers(0, 1, buffers);

  context->Draw(4, 0);
  context->RSSetState(rasterizerState.Get());
}

void Renderer::Impl::DrawCube(const math::Matrix &viewMatrix,
                              const CubeState &cube,
                              const math::Float4 &color) {
  const math::Matrix world =
      math::Scaling(cube.extents.x * 2.0f, cube.extents.y * 2.0f,
                    cube.extents.z * 2.0f) *
      math::RotationRollPitchYaw(cube.rotation.x, cube.rotation.y,
                                 cube.rotation.z) *
      math::Translation(cube.position.x, cube.position.y, cube.position.z);

  const math::Matrix projection = math::PerspectiveFovLH(
      math::ToRadians(60.0f),
      static_cast<float>(width) / static_cast<float>(std::max(height, 1u)),
      0.1f, 100.0f);

  ObjectConstants constants = {};
  constants.worldViewProjection =
      math::Transpose(world * viewMatrix * projection);
  constants.world = math::Transpose(world);
  constants.color = color;
  constants.lightDirection = math::MakeFloat4(-0.5f, -1.0f, 0.4f, 0.0f);

  D3D11_MAPPED_SUBRESOURCE mapped = {};
  ThrowIfFailed(context->Map(objectConstantBuffer.Get(), 0,
                             D3D11_MAP_WRITE_DISCARD, 0, &mapped),
                "Map(object constants)");
  std::memcpy(mapped.pData, &constants, sizeof(constants));
  context->Unmap(objectConstantBuffer.Get(), 0);

  context->DrawIndexed(indexCount, 0, 0);
}

void Renderer::Impl::DrawSelectionOverlay(const Camera &camera,
                                          const CubeState &obstacleCube,
                                          const CubeState &playerCube,
                                          bool obstacleSelected,
                                          bool playerSelected) {
  if (!obstacleSelected && !playerSelected) {
    return;
  }

  const math::Matrix viewMatrix = camera.GetViewMatrix();

  ID3D11RenderTargetView *renderTargets[] = {backBufferRenderTargetView.Get()};
  context->OMSetRenderTargets(1, renderTargets, nullptr);
  context->RSSetViewports(1, &viewport);
  context->RSSetState(selectionRasterizerState.Get());
  context->OMSetDepthStencilState(disabledDepthStencilState.Get(), 0);

  constexpr float blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  context->OMSetBlendState(blendState.Get(), blendFactor, 0xFFFFFFFF);

  UINT stride = sizeof(Vertex);
  UINT offset = 0;
  ID3D11Buffer *vertexBuffers[] = {vertexBuffer.Get()};
  context->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);
  context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
  context->IASetInputLayout(inputLayout.Get());
  context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  context->VSSetShader(vertexShader.Get(), nullptr, 0);
  context->PSSetShader(pixelShader.Get(), nullptr, 0);

  ID3D11Buffer *constantBuffers[] = {objectConstantBuffer.Get()};
  context->VSSetConstantBuffers(0, 1, constantBuffers);
  context->PSSetConstantBuffers(0, 1, constantBuffers);

  if (obstacleSelected) {
    DrawSelectionCube(viewMatrix, obstacleCube);
  }
  if (playerSelected) {
    DrawSelectionCube(viewMatrix, playerCube);
  }

  context->RSSetState(rasterizerState.Get());
}

void Renderer::Impl::DrawSelectionCube(const math::Matrix &viewMatrix,
                                       const CubeState &cube) {
  CubeState selectionCube = cube;
  selectionCube.extents.x *= 1.03f;
  selectionCube.extents.y *= 1.03f;
  selectionCube.extents.z *= 1.03f;
  DrawCube(viewMatrix, selectionCube, math::MakeFloat4(1.0f, 0.6f, 0.1f, 1.0f));
}

void Renderer::Impl::BeginScenePass() {
  const float clearColor[] = {0.0f, 0.0f, 0.0f, 1.0f};
  context->ClearRenderTargetView(sceneColorRenderTargetView.Get(), clearColor);
  context->ClearDepthStencilView(
      depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

  ID3D11RenderTargetView *renderTargets[] = {sceneColorRenderTargetView.Get()};
  context->OMSetRenderTargets(1, renderTargets, depthStencilView.Get());
  context->RSSetViewports(1, &viewport);
  context->RSSetState(rasterizerState.Get());
  context->OMSetDepthStencilState(depthStencilState.Get(), 0);

  constexpr float blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  context->OMSetBlendState(blendState.Get(), blendFactor, 0xFFFFFFFF);
}

void Renderer::Impl::ResolveToBackBuffer(ID3D11ShaderResourceView *source,
                                         std::uint32_t sourceWidth,
                                         std::uint32_t sourceHeight) {
  switch (antiAliasingMode) {
  case AntiAliasingMode::None:
    UpdatePostProcessConstants(sourceWidth, sourceHeight, 0.0f, 0.0f);
    ExecuteFullscreenPass(copyPixelShader.Get(), source);
    break;
  case AntiAliasingMode::FXAA:
    UpdatePostProcessConstants(sourceWidth, sourceHeight, 0.125f, 0.75f);
    ExecuteFullscreenPass(fxaaPixelShader.Get(), source);
    break;
  case AntiAliasingMode::SMAA:
    UpdatePostProcessConstants(sourceWidth, sourceHeight, 0.08f, 1.0f);
    ExecuteFullscreenPass(smaaEdgePixelShader.Get(), source);
    UpdatePostProcessConstants(sourceWidth, sourceHeight, 0.08f, 1.0f);
    ExecuteFullscreenPass(smaaBlendPixelShader.Get(), source,
                          smaaEdgeShaderResourceView.Get());
    break;
  default:
    ExecuteFullscreenPass(copyPixelShader.Get(), source);
    break;
  }
}

void Renderer::Impl::ExecuteFullscreenPass(
    ID3D11PixelShader *pixelShader, ID3D11ShaderResourceView *primarySource,
    ID3D11ShaderResourceView *secondarySource) {
  const bool writeEdges = pixelShader == smaaEdgePixelShader.Get();
  ID3D11RenderTargetView *target = writeEdges
                                       ? smaaEdgeRenderTargetView.Get()
                                       : backBufferRenderTargetView.Get();
  const float clearColor[] = {0.0f, 0.0f, 0.0f, 1.0f};
  context->ClearRenderTargetView(target, clearColor);

  ID3D11RenderTargetView *renderTargets[] = {target};
  context->OMSetRenderTargets(1, renderTargets, nullptr);
  context->RSSetViewports(1, &viewport);
  context->OMSetDepthStencilState(disabledDepthStencilState.Get(), 0);

  UINT stride = sizeof(FullscreenVertex);
  UINT offset = 0;
  ID3D11Buffer *vertexBuffers[] = {fullscreenVertexBuffer.Get()};
  context->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);
  context->IASetInputLayout(fullscreenInputLayout.Get());
  context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
  context->VSSetShader(fullscreenVertexShader.Get(), nullptr, 0);
  context->PSSetShader(pixelShader, nullptr, 0);

  ID3D11Buffer *postBuffers[] = {postProcessConstantBuffer.Get()};
  context->PSSetConstantBuffers(0, 1, postBuffers);

  ID3D11SamplerState *samplers[] = {linearClampSamplerState.Get()};
  context->PSSetSamplers(0, 1, samplers);

  ID3D11ShaderResourceView *shaderResources[] = {primarySource,
                                                 secondarySource};
  context->PSSetShaderResources(0, secondarySource ? 2u : 1u, shaderResources);
  context->Draw(4, 0);

  ID3D11ShaderResourceView *nullResources[2] = {nullptr, nullptr};
  context->PSSetShaderResources(0, 2, nullResources);
}

void Renderer::Impl::UpdatePostProcessConstants(std::uint32_t sourceWidth,
                                                std::uint32_t sourceHeight,
                                                float edgeThreshold,
                                                float blendStrength) {
  PostProcessConstants constants = {};
  constants.texelSize =
      math::MakeFloat2(1.0f / static_cast<float>(std::max(sourceWidth, 1u)),
                       1.0f / static_cast<float>(std::max(sourceHeight, 1u)));
  constants.screenSize =
      math::MakeFloat2(static_cast<float>(width), static_cast<float>(height));
  constants.parameters =
      math::MakeFloat4(edgeThreshold, blendStrength, 0.0f, 0.0f);

  D3D11_MAPPED_SUBRESOURCE mapped = {};
  ThrowIfFailed(context->Map(postProcessConstantBuffer.Get(), 0,
                             D3D11_MAP_WRITE_DISCARD, 0, &mapped),
                "Map(postprocess constants)");
  std::memcpy(mapped.pData, &constants, sizeof(constants));
  context->Unmap(postProcessConstantBuffer.Get(), 0);
}

void Renderer::Impl::UpdateRayTraceTexture(const Camera &camera,
                                           const CubeState &obstacleCube,
                                           const CubeState &playerCube,
                                           bool collisionActive) {

  const math::Float4 obstacleColor =
      collisionActive ? math::MakeFloat4(0.90f, 0.28f, 0.25f, 1.0f)
                      : math::MakeFloat4(0.20f, 0.71f, 0.93f, 1.0f);
  const math::Float4 playerColor =
      collisionActive ? math::MakeFloat4(0.99f, 0.83f, 0.20f, 1.0f)
                      : math::MakeFloat4(0.33f, 0.91f, 0.48f, 1.0f);
  gpuRayTracer.Render(context.Get(), camera, obstacleCube, obstacleColor,
                      playerCube, playerColor);
}
