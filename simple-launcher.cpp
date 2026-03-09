#define UNICODE
#define _UNICODE
#include <windows.h>
#include <vector>
#include <string>
#include <algorithm>

using namespace std;

struct Game {
    wstring name;
    wstring exePath;
};

vector<Game> games;
HWND hListBox = NULL;

// Find main executable in game folder
wstring FindGameExe(const wstring& folder) {
    WIN32_FIND_DATAW findData;
    wstring searchPath = folder + L"\\*.exe";
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            wstring fileName = findData.cFileName;
            wstring lowerName = fileName;
            transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            
            // Skip utility executables
            if (lowerName.find(L"unins") == wstring::npos &&
                lowerName.find(L"crash") == wstring::npos &&
                lowerName.find(L"setup") == wstring::npos) {
                FindClose(hFind);
                return folder + L"\\" + fileName;
            }
        } while (FindNextFileW(hFind, &findData));
        FindClose(hFind);
    }
    return L"";
}

// Scan games folder
void ScanGames() {
    games.clear();
    
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(L"E:\\games\\*", &findData);
    
    if (hFind == INVALID_HANDLE_VALUE) {
        MessageBoxW(NULL, L"Could not access E:\\games folder!", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    do {
        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            wcscmp(findData.cFileName, L".") != 0 &&
            wcscmp(findData.cFileName, L"..") != 0) {
            
            Game game;
            game.name = findData.cFileName;
            game.exePath = FindGameExe(wstring(L"E:\\games\\") + findData.cFileName);
            
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
    
    // Fill listbox
    if (hListBox) {
        SendMessageW(hListBox, LB_RESETCONTENT, 0, 0);
        for (const auto& game : games) {
            SendMessageW(hListBox, LB_ADDSTRING, 0, (LPARAM)game.name.c_str());
        }
    }
}

// Launch selected game
void LaunchGame(int index) {
    if (index >= 0 && index < (int)games.size()) {
        ShellExecuteW(NULL, L"open", games[index].exePath.c_str(), NULL, NULL, SW_SHOW);
    }
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            // Create listbox
            hListBox = CreateWindowW(L"LISTBOX", NULL,
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_HASSTRINGS,
                10, 50, 760, 500,
                hWnd, (HMENU)1, NULL, NULL);
            
            // Create play button
            CreateWindowW(L"BUTTON", L"Play Selected Game",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                10, 10, 200, 30,
                hWnd, (HMENU)2, NULL, NULL);
            
            // Create refresh button
            CreateWindowW(L"BUTTON", L"Refresh List",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                220, 10, 150, 30,
                hWnd, (HMENU)3, NULL, NULL);
            
            // Scan games
            ScanGames();
            return 0;
        }
        
        case WM_COMMAND: {
            if (LOWORD(wParam) == 2) {
                // Play button
                int sel = (int)SendMessageW(hListBox, LB_GETCURSEL, 0, 0);
                if (sel != LB_ERR) {
                    LaunchGame(sel);
                }
            } else if (LOWORD(wParam) == 3) {
                // Refresh button
                ScanGames();
            } else if (LOWORD(wParam) == 1 && HIWORD(wParam) == LBN_DBLCLK) {
                // Double-click on listbox
                int sel = (int)SendMessageW(hListBox, LB_GETCURSEL, 0, 0);
                if (sel != LB_ERR) {
                    LaunchGame(sel);
                }
            }
            return 0;
        }
        
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    // Register window class
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"GameLauncher";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(1));
    
    RegisterClassW(&wc);
    
    // Create window
    HWND hWnd = CreateWindowExW(
        0, L"GameLauncher", L"Game Launcher - E:\\games",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, hInstance, NULL
    );
    
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    
    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return 0;
}
