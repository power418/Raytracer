#include "../include/DemoApp.hpp"

#include <Windows.h>

#include <consoleapi.h>
#include <cstdio>
#include <cstdlib>
#include <exception>

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand)
{
  try
  {
#if defined(_DEBUG)
    AllocConsole();
    SetConsoleTitleW(L"D3D12Engine - Debug Console");

    // Redirect stdout/stderr to the allocated console
    FILE* fpOut = nullptr;
    FILE* fpErr = nullptr;
    freopen_s(&fpOut, "CONOUT$", "w", stdout);
    freopen_s(&fpErr, "CONOUT$", "w", stderr);

    std::printf("========================================\n");
    std::printf("  D3D12Engine - Debug Console\n");
    std::printf("========================================\n\n");
    std::printf("[main] Application starting...\n");
#endif

    DemoApp app(instance);
    std::printf("[main] DemoApp created successfully\n");
    std::printf("[main] Entering main loop (showCommand=%d)\n", showCommand);
    const int result = app.Run(showCommand);

    std::printf("[main] Application exiting with code %d\n", result);
    return result;
  }
  catch (const std::exception& exception)
  {
    std::fprintf(stderr, "[main] FATAL EXCEPTION: %s\n", exception.what());
    MessageBoxA(nullptr, exception.what(), "Fatal Error", MB_OK | MB_ICONERROR);
    return EXIT_FAILURE;
  }
}
