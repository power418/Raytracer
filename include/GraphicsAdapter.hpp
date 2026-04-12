#pragma once

#define NOMINMAX
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_6.h>

#include <cstddef>
#include <cstdint>
#include <string>

namespace gfx
{
struct GraphicsAdapterInfo
{
  std::wstring description;
  std::size_t dedicatedVideoMemory = 0;
  bool highPerformanceSelectionAvailable = false;
  bool discreteLikely = false;
};

HRESULT CreateHighPerformanceDeviceAndSwapChain(
  HWND windowHandle,
  std::uint32_t width,
  std::uint32_t height,
  UINT deviceFlags,
  IDXGISwapChain** swapChain,
  ID3D11Device** device,
  ID3D11DeviceContext** deviceContext,
  GraphicsAdapterInfo* adapterInfo);
}  // namespace gfx
