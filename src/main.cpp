#include "DemoApp.hpp"

#include <Windows.h>

#include <cstdlib>
#include <exception>

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand)
{
  try
  {
    DemoApp app(instance);
    return app.Run(showCommand);
  }
  catch (const std::exception& exception)
  {
    MessageBoxA(nullptr, exception.what(), "Fatal Error", MB_OK | MB_ICONERROR);
    return EXIT_FAILURE;
  }
}
