#define UNICODE
#define _UNICODE
#include <windows.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <wininet.h>
#include <string>
#include <vector>
#include <algorithm>
#include <thread>
#include <gdiplus.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;
using namespace std;

struct Game {
    wstring name;
    wstring path;
    wstring exePath;
    wstring imagePath;
    HBITMAP hBitmap;
};

vector<Game> games;
HWND hMainWnd = NULL;
HWND hScrollWnd = NULL;
HWND hSearchBox = NULL;
vector<HWND> gameButtons;
vector<HWND> gameImages;
HFONT hTitleFont = NULL;
HFONT hButtonFont = NULL;
int scrollPos = 0;
wstring searchQuery = L"";

// Download image from URL
bool DownloadImage(const wstring& gameName, const wstring& outputPath) {
    wstring searchUrl = L"https://www.steamgriddb.com/api/v2/search/autocomplete/" + gameName;
    // Simplified - in production, use proper API
    // For now, just create placeholder
    return false;
}

// Find main executable in game folder
wstring FindGameExe(const wstring& folder) {
    WIN32_FIND_DATAW findData;
    wstring searchPath = folder + L"\\*.exe";
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    
    vector<wstring> exeFiles;
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            wstring fileName = findData.cFileName;
            wstring lowerName = fileName;
            transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            
            // Skip common utility executables
            if (lowerName.find(L"unins") == wstring::npos &&
                lowerName.find(L"crash") == wstring::npos &&
                lowerName.find(L"report") == wstring::npos &&
                lowerName.find(L"setup") == wstring::npos &&
                lowerName.find(L"launcher") == wstring::npos &&
                lowerName.find(L"config") == wstring::npos) {
                exeFiles.push_back(folder + L"\\" + fileName);
            }
        } while (FindNextFileW(hFind, &findData));
        FindClose(hFind);
    }
    
    // Return first valid exe
    if (!exeFiles.empty()) {
        return exeFiles[0];
    }
    return L"";
}

// Load image as HBITMAP
HBITMAP LoadImageFile(const wstring& path, int width, int height) {
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    
    Bitmap* bitmap = new Bitmap(path.c_str());
    if (bitmap->GetLastStatus() != Ok) {
        delete bitmap;
        return NULL;
    }
    
    HBITMAP hBitmap = NULL;
    bitmap->GetHBITMAP(Color(240, 240, 240), &hBitmap);
    delete bitmap;
    
    return hBitmap;
}

// Scan games folder
void ScanGames() {
    games.clear();
    
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(L"E:\\games\\*", &findData);
    
    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }
    
    do {
        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            wcscmp(findData.cFileName, L".") != 0 &&
            wcscmp(findData.cFileName, L"..") != 0) {
            
            Game game;
            game.name = findData.cFileName;
            game.path = wstring(L"E:\\games\\") + findData.cFileName;
            game.exePath = FindGameExe(game.path);
            game.imagePath = L""; // Will be populated later
            game.hBitmap = NULL;
            
            if (!game.exePath.empty()) {
                games.push_back(game);
            }
        }
    } while (FindNextFileW(hFind, &findData));
    
    FindClose(hFind);
    
    // Sort alphabetically
    sort(games.begin(), games.end(), [](const Game& a, const Game& b) {
        return a.name < b.name;
    });
}

// Create game buttons UI
void CreateGameButtons(HWND hWnd) {
    // Clear existing buttons
    for (auto btn : gameButtons) DestroyWindow(btn);
    for (auto img : gameImages) DestroyWindow(img);
    gameButtons.clear();
    gameImages.clear();
    
    int y = 20;
    int buttonWidth = 760;
    int buttonHeight = 60;
    int spacing = 10;
    
    for (size_t i = 0; i < games.size(); i++) {
        // Filter by search query
        if (!searchQuery.empty()) {
            wstring lowerName = games[i].name;
            wstring lowerQuery = searchQuery;
            transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
            if (lowerName.find(lowerQuery) == wstring::npos) {
                continue;
            }
        }
        
        // Create play button
        HWND hButton = CreateWindowW(L"BUTTON", games[i].name.c_str(),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_LEFT,
            20, y - scrollPos, buttonWidth, buttonHeight,
            hWnd, (HMENU)(1000 + i), NULL, NULL);
        
        SendMessage(hButton, WM_SETFONT, (WPARAM)hButtonFont, TRUE);
        gameButtons.push_back(hButton);
        
        y += buttonHeight + spacing;
    }
}

// Launch game
void LaunchGame(int index) {
    if (index >= 0 && index < (int)games.size()) {
        ShellExecuteW(NULL, L"open", games[index].exePath.c_str(), NULL, NULL, SW_SHOW);
    }
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            // Create search box
            hSearchBox = CreateWindowW(L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT,
                20, 20, 600, 30,
                hWnd, NULL, NULL, NULL);
            SendMessage(hSearchBox, WM_SETFONT, (WPARAM)hButtonFont, TRUE);
            
            // Create refresh button
            CreateWindowW(L"BUTTON", L"↻ Refresh",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                640, 20, 140, 30,
                hWnd, (HMENU)999, NULL, NULL);
            
            // Create scroll container
            hScrollWnd = CreateWindowW(L"STATIC", L"",
                WS_CHILD | WS_VISIBLE,
                0, 70, 800, 530,
                hWnd, NULL, NULL, NULL);
            
            // Scan games
            ScanGames();
            CreateGameButtons(hScrollWnd);
            
            return 0;
        }
        
        case WM_COMMAND: {
            int id = LOWORD(wParam);
            
            if (id == 999) {
                // Refresh button
                ScanGames();
                CreateGameButtons(hScrollWnd);
            } else if (id >= 1000 && id < 1000 + (int)games.size()) {
                // Game button
                LaunchGame(id - 1000);
            } else if (HIWORD(wParam) == EN_CHANGE && (HWND)lParam == hSearchBox) {
                // Search box changed
                wchar_t buffer[256];
                GetWindowTextW(hSearchBox, buffer, 256);
                searchQuery = buffer;
                scrollPos = 0;
                CreateGameButtons(hScrollWnd);
            }
            return 0;
        }
        
        case WM_MOUSEWHEEL: {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            scrollPos -= delta / 4;
            if (scrollPos < 0) scrollPos = 0;
            CreateGameButtons(hScrollWnd);
            return 0;
        }
        
        case WM_CTLCOLORSTATIC: {
            HDC hdcStatic = (HDC)wParam;
            SetBkColor(hdcStatic, RGB(240, 240, 240));
            return (LRESULT)GetStockObject(NULL_BRUSH);
        }
        
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    // Initialize GDI+
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    
    // Create fonts
    hTitleFont = CreateFontW(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    
    hButtonFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    
    // Register window class
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"GameLauncher";
    wc.hbrBackground = CreateSolidBrush(RGB(240, 240, 240));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(1));
    
    RegisterClassW(&wc);
    
    // Create window
    hMainWnd = CreateWindowExW(
        0,
        L"GameLauncher",
        L"Game Launcher - E:\\games",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 820, 650,
        NULL, NULL, hInstance, NULL
    );
    
    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);
    
    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    GdiplusShutdown(gdiplusToken);
    return 0;
}
