#include "../include/GraphicsAdapter.hpp"

#include <wrl/client.h>

#include <array>
#include <cstdint>
#include <cstdio>

using Microsoft::WRL::ComPtr;

extern "C"
{
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 0x00000001;
}

namespace
{
constexpr std::uint64_t kDiscreteMemoryThresholdBytes = 256ull * 1024ull * 1024ull;

struct AdapterCandidate
{
  ComPtr<IDXGIAdapter1> adapter;
  DXGI_ADAPTER_DESC1 description = {};
};

bool IsSoftwareAdapter(const DXGI_ADAPTER_DESC1& description)
{
  return (description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0;
}

bool LooksDiscrete(const DXGI_ADAPTER_DESC1& description)
{
  return description.DedicatedVideoMemory >= kDiscreteMemoryThresholdBytes;
}

void TryAddAdapter(ComPtr<IDXGIAdapter1> adapter, AdapterCandidate* bestCandidate)
{
  DXGI_ADAPTER_DESC1 description = {};
  if (!adapter || FAILED(adapter->GetDesc1(&description)) || IsSoftwareAdapter(description))
  {
    return;
  }

  std::printf("[GraphicsAdapter]   Found adapter: %ls (VRAM: %zu MB)\n",
              description.Description,
              static_cast<std::size_t>(description.DedicatedVideoMemory / (1024 * 1024)));

  if (!bestCandidate->adapter || description.DedicatedVideoMemory > bestCandidate->description.DedicatedVideoMemory)
  {
    bestCandidate->adapter = adapter;
    bestCandidate->description = description;
  }
}

AdapterCandidate SelectBestHardwareAdapter(IDXGIFactory1* factory, bool* highPerformanceSelectionAvailable)
{
  AdapterCandidate bestCandidate = {};
  *highPerformanceSelectionAvailable = false;

  std::printf("[GraphicsAdapter] Enumerating GPU adapters...\n");

  ComPtr<IDXGIFactory6> factory6;
  if (SUCCEEDED(factory->QueryInterface(IID_PPV_ARGS(factory6.GetAddressOf()))))
  {
    *highPerformanceSelectionAvailable = true;
    std::printf("[GraphicsAdapter] IDXGIFactory6 available (high-performance GPU selection)\n");

    for (UINT adapterIndex = 0;; ++adapterIndex)
    {
      ComPtr<IDXGIAdapter1> adapter;
      const HRESULT hr = factory6->EnumAdapterByGpuPreference(
        adapterIndex,
        DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
        IID_PPV_ARGS(adapter.GetAddressOf()));

      if (hr == DXGI_ERROR_NOT_FOUND)
      {
        break;
      }

      if (SUCCEEDED(hr))
      {
        TryAddAdapter(adapter, &bestCandidate);
      }
    }
  }
  else
  {
    std::printf("[GraphicsAdapter] IDXGIFactory6 not available, using legacy enumeration\n");
  }

  if (bestCandidate.adapter)
  {
    std::printf("[GraphicsAdapter] Selected adapter: %ls\n", bestCandidate.description.Description);
    return bestCandidate;
  }

  for (UINT adapterIndex = 0;; ++adapterIndex)
  {
    ComPtr<IDXGIAdapter1> adapter;
    const HRESULT hr = factory->EnumAdapters1(adapterIndex, adapter.GetAddressOf());
    if (hr == DXGI_ERROR_NOT_FOUND)
    {
      break;
    }

    if (SUCCEEDED(hr))
    {
      TryAddAdapter(adapter, &bestCandidate);
    }
  }

  if (bestCandidate.adapter)
  {
    std::printf("[GraphicsAdapter] Selected adapter (legacy): %ls\n", bestCandidate.description.Description);
  }
  else
  {
    std::fprintf(stderr, "[GraphicsAdapter] ERROR: No suitable GPU adapter found!\n");
  }

  return bestCandidate;
}

HRESULT CreateDeviceAndSwapChainForAdapter(
  IDXGIAdapter1* adapter,
  HWND windowHandle,
  std::uint32_t width,
  std::uint32_t height,
  UINT deviceFlags,
  IDXGISwapChain** swapChain,
  ID3D11Device** device,
  ID3D11DeviceContext** deviceContext)
{
  DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
  swapChainDesc.BufferDesc.Width = width;
  swapChainDesc.BufferDesc.Height = height;
  swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
  swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
  swapChainDesc.SampleDesc.Count = 1;
  swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDesc.BufferCount = 2;
  swapChainDesc.OutputWindow = windowHandle;
  swapChainDesc.Windowed = TRUE;
  swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

  constexpr std::array<D3D_FEATURE_LEVEL, 4> requestedLevelsWith11_1 = {
    D3D_FEATURE_LEVEL_11_1,
    D3D_FEATURE_LEVEL_11_0,
    D3D_FEATURE_LEVEL_10_1,
    D3D_FEATURE_LEVEL_10_0,
  };

  constexpr std::array<D3D_FEATURE_LEVEL, 3> requestedLevelsLegacy = {
    D3D_FEATURE_LEVEL_11_0,
    D3D_FEATURE_LEVEL_10_1,
    D3D_FEATURE_LEVEL_10_0,
  };

  D3D_FEATURE_LEVEL createdLevel = D3D_FEATURE_LEVEL_11_0;
  HRESULT hr = D3D11CreateDeviceAndSwapChain(
    adapter,
    D3D_DRIVER_TYPE_UNKNOWN,
    nullptr,
    deviceFlags,
    requestedLevelsWith11_1.data(),
    static_cast<UINT>(requestedLevelsWith11_1.size()),
    D3D11_SDK_VERSION,
    &swapChainDesc,
    swapChain,
    device,
    &createdLevel,
    deviceContext);

  if (hr == E_INVALIDARG)
  {
    hr = D3D11CreateDeviceAndSwapChain(
      adapter,
      D3D_DRIVER_TYPE_UNKNOWN,
      nullptr,
      deviceFlags,
      requestedLevelsLegacy.data(),
      static_cast<UINT>(requestedLevelsLegacy.size()),
      D3D11_SDK_VERSION,
      &swapChainDesc,
      swapChain,
      device,
      &createdLevel,
      deviceContext);
  }

  return hr;
}
}  // namespace

HRESULT gfx::CreateHighPerformanceDeviceAndSwapChain(
  HWND windowHandle,
  std::uint32_t width,
  std::uint32_t height,
  UINT deviceFlags,
  IDXGISwapChain** swapChain,
  ID3D11Device** device,
  ID3D11DeviceContext** deviceContext,
  GraphicsAdapterInfo* adapterInfo)
{
  if (!swapChain || !device || !deviceContext)
  {
    return E_INVALIDARG;
  }

  ComPtr<IDXGIFactory1> factory;
  HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(factory.GetAddressOf()));
  if (FAILED(hr))
  {
    return hr;
  }

  bool highPerformanceSelectionAvailable = false;
  const AdapterCandidate selectedAdapter = SelectBestHardwareAdapter(factory.Get(), &highPerformanceSelectionAvailable);
  if (!selectedAdapter.adapter)
  {
    return DXGI_ERROR_NOT_FOUND;
  }

  hr = CreateDeviceAndSwapChainForAdapter(
    selectedAdapter.adapter.Get(),
    windowHandle,
    width,
    height,
    deviceFlags,
    swapChain,
    device,
    deviceContext);

  if (FAILED(hr) && (deviceFlags & D3D11_CREATE_DEVICE_DEBUG) != 0)
  {
    hr = CreateDeviceAndSwapChainForAdapter(
      selectedAdapter.adapter.Get(),
      windowHandle,
      width,
      height,
      deviceFlags & ~D3D11_CREATE_DEVICE_DEBUG,
      swapChain,
      device,
      deviceContext);
  }

  if (SUCCEEDED(hr) && adapterInfo)
  {
    adapterInfo->description = selectedAdapter.description.Description;
    adapterInfo->dedicatedVideoMemory = static_cast<std::size_t>(selectedAdapter.description.DedicatedVideoMemory);
    adapterInfo->highPerformanceSelectionAvailable = highPerformanceSelectionAvailable;
    adapterInfo->discreteLikely = LooksDiscrete(selectedAdapter.description);
    std::printf("[GraphicsAdapter] GPU: %ls\n", adapterInfo->description.c_str());
    std::printf("[GraphicsAdapter] VRAM: %zu MB\n", adapterInfo->dedicatedVideoMemory / (1024 * 1024));
    std::printf("[GraphicsAdapter] Discrete GPU: %s\n", adapterInfo->discreteLikely ? "Yes" : "No");
    std::printf("[GraphicsAdapter] High-perf selection: %s\n", adapterInfo->highPerformanceSelectionAvailable ? "Yes" : "No");
  }

  return hr;
}
