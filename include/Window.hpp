#pragma once

#define NOMINMAX
#include <Windows.h>

#include <cstdint>
#include <string>
#include <string_view>

class IWindowEvents
{
public:
  virtual ~IWindowEvents() = default;

  virtual void OnResize(std::uint32_t width, std::uint32_t height) = 0;
  virtual void OnKeyDown(WPARAM key) = 0;
  virtual void OnCommand(WPARAM wParam, LPARAM lParam) = 0;
};

class Window
{
public:
  Window(HINSTANCE instance,
         IWindowEvents& events,
         std::wstring_view title,
         std::uint32_t clientWidth,
         std::uint32_t clientHeight);
  ~Window();

  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;

  bool ProcessMessages() const;
  void Show(int showCommand) const;

  HWND GetHandle() const;
  std::uint32_t GetClientWidth() const;
  std::uint32_t GetClientHeight() const;

private:
  void RegisterWindowClass();
  void CreateAppWindow();
  LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);

  static LRESULT CALLBACK WindowProcSetup(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
  static LRESULT CALLBACK WindowProcThunk(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

private:
  HINSTANCE instance_ = nullptr;
  IWindowEvents& events_;
  std::wstring title_;
  std::uint32_t clientWidth_ = 1280;
  std::uint32_t clientHeight_ = 720;
  HWND windowHandle_ = nullptr;
  ATOM classAtom_ = 0;
};
