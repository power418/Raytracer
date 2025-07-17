#if defined(_WIN32) || defined (_WIN64)
#define UNICODE
#include <Windows.h>
#endif

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR cmdLine, INT cmdShow)
{
  return MessageBoxW(nullptr, L"Hello, World!", L"Message", MB_OKCANCEL);
}