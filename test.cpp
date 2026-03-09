#include <windows.h>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    MessageBoxW(NULL, L"Hello World!", L"Test", MB_OK);
    return 0;
}
