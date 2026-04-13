#include "Window.hpp"

#include <windowsx.h>

#include <stdexcept>

namespace
{
constexpr wchar_t kWindowClassName[] = L"D3D11CollisionCubeWindow";
}

Window::Window(HINSTANCE instance,
               IWindowEvents& events,
               std::wstring_view title,
               std::uint32_t clientWidth,
               std::uint32_t clientHeight)
  : instance_(instance),
    events_(events),
    title_(title),
    clientWidth_(clientWidth),
    clientHeight_(clientHeight)
{
  RegisterWindowClass();
  CreateAppWindow();
}

Window::~Window()
{
  if (windowHandle_)
  {
    DestroyWindow(windowHandle_);
  }

  if (classAtom_ != 0)
  {
    UnregisterClassW(kWindowClassName, instance_);
  }
}

bool Window::ProcessMessages() const
{
  MSG message = {};
  while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
  {
    if (message.message == WM_QUIT)
    {
      return false;
    }

    TranslateMessage(&message);
    DispatchMessageW(&message);
  }

  return true;
}

void Window::Show(int showCommand) const
{
  ShowWindow(windowHandle_, showCommand);
  UpdateWindow(windowHandle_);
}

HWND Window::GetHandle() const
{
  return windowHandle_;
}

std::uint32_t Window::GetClientWidth() const
{
  return clientWidth_;
}

std::uint32_t Window::GetClientHeight() const
{
  return clientHeight_;
}

void Window::RegisterWindowClass()
{
  WNDCLASSEXW windowClass = {};
  windowClass.cbSize = sizeof(windowClass);
  windowClass.lpfnWndProc = &Window::WindowProcSetup;
  windowClass.hInstance = instance_;
  windowClass.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
  windowClass.lpszClassName = kWindowClassName;

  classAtom_ = RegisterClassExW(&windowClass);
  if (classAtom_ == 0)
  {
    throw std::runtime_error("RegisterClassExW failed");
  }
}

void Window::CreateAppWindow()
{
  RECT rect = {0, 0, static_cast<LONG>(clientWidth_), static_cast<LONG>(clientHeight_)};
  AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

  windowHandle_ = CreateWindowExW(
    0,
    kWindowClassName,
    title_.c_str(),
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    rect.right - rect.left,
    rect.bottom - rect.top,
    nullptr,
    nullptr,
    instance_,
    this);

  if (!windowHandle_)
  {
    throw std::runtime_error("CreateWindowExW failed");
  }
}

LRESULT Window::HandleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_SIZE:
    clientWidth_ = static_cast<std::uint32_t>(LOWORD(lParam));
    clientHeight_ = static_cast<std::uint32_t>(HIWORD(lParam));
    events_.OnResize(clientWidth_, clientHeight_);
    return 0;
  case WM_KEYDOWN:
    events_.OnKeyDown(wParam);
    return 0;
  case WM_LBUTTONDOWN:
    events_.OnLeftMouseDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    return 0;
  case WM_RBUTTONDOWN:
    events_.OnRightMouseDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    return 0;
  case WM_COMMAND:
    events_.OnCommand(wParam, lParam);
    return 0;
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  default:
    return DefWindowProcW(windowHandle_, message, wParam, lParam);
  }
}

LRESULT CALLBACK Window::WindowProcSetup(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
  if (message == WM_NCCREATE)
  {
    const auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
    auto* windowInstance = static_cast<Window*>(createStruct->lpCreateParams);
    SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(windowInstance));
    SetWindowLongPtrW(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&Window::WindowProcThunk));
    windowInstance->windowHandle_ = window;
    return windowInstance->HandleMessage(message, wParam, lParam);
  }

  return DefWindowProcW(window, message, wParam, lParam);
}

LRESULT CALLBACK Window::WindowProcThunk(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
  auto* windowInstance = reinterpret_cast<Window*>(GetWindowLongPtrW(window, GWLP_USERDATA));
  return windowInstance ? windowInstance->HandleMessage(message, wParam, lParam)
                        : DefWindowProcW(window, message, wParam, lParam);
}
