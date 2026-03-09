#define UNICODE
#define _UNICODE
#include <windows.h>
#include <commctrl.h>
#include <vector>
#include <string>
#include <algorithm>

#pragma comment(lib, "comctl32.lib")

using namespace std;

struct Game {
    wstring name;
    wstring exePath;
    bool hasMultipleExes;
};

vector<Game> games;
HWND hListBox = NULL;
HWND hSearchBox = NULL;
HWND hStatusBar = NULL;
HFONT hMainFont = NULL;

// Find all executables in game folder
vector<wstring> FindAllExes(const wstring& folder) {
    vector<wstring> exes;
    WIN32_FIND_DATAW findData;
    wstring searchPath = folder + L"\\*.exe";
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            wstring fileName = findData.cFileName;
            wstring lowerName = fileName;
            transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            
            // Skip known utility executables
            if (lowerName.find(L"unins") == wstring::npos &&
                lowerName.find(L"crash") == wstring::npos &&
                lowerName.find(L"report") == wstring::npos &&
                lowerName.find(L"setup") == wstring::npos &&
                lowerName.find(L"update") == wstring::npos &&
                lowerName.find(L"config") == wstring::npos &&
                lowerName.find(L"redist") == wstring::npos &&
                lowerName.find(L"vcredist") == wstring::npos &&
                lowerName.find(L"dxsetup") == wstring::npos) {
                exes.push_back(folder + L"\\" + fileName);
            }
        } while (FindNextFileW(hFind, &findData));
        FindClose(hFind);
    }
    return exes;
}

// Scan games folder
void ScanGames() {
    games.clear();
    
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(L"E:\\games\\*", &findData);
    
    if (hFind == INVALID_HANDLE_VALUE) {
        MessageBoxW(NULL, L"Could not access E:\\games folder!\n\nMake sure the folder exists and you have permission to read it.",
                   L"Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    int folderCount = 0;
    int gameCount = 0;
    
    do {
        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            wcscmp(findData.cFileName, L".") != 0 &&
            wcscmp(findData.cFileName, L"..") != 0) {
            
            folderCount++;
            wstring gamePath = wstring(L"E:\\games\\") + findData.cFileName;
            vector<wstring> exes = FindAllExes(gamePath);
            
            if (!exes.empty()) {
                Game game;
                game.name = findData.cFileName;
                game.exePath = exes[0];  // Use first exe by default
                game.hasMultipleExes = exes.size() > 1;
                games.push_back(game);
                gameCount++;
            }
        }
    } while (FindNextFileW(hFind, &findData));
    
    FindClose(hFind);
    
    // Sort alphabetically
    sort(games.begin(), games.end(), [](const Game& a, const Game& b) {
        return _wcsicmp(a.name.c_str(), b.name.c_str()) < 0;
    });
    
    // Update listbox
    if (hListBox) {
        SendMessageW(hListBox, LB_RESETCONTENT, 0, 0);
        for (const auto& game : games) {
            wstring displayName = game.name;
            if (game.hasMultipleExes) {
                displayName += L" *";
            }
            SendMessageW(hListBox, LB_ADDSTRING, 0, (LPARAM)displayName.c_str());
        }
    }
    
    // Update status bar
    if (hStatusBar) {
        wstring status = L"Found " + to_wstring(gameCount) + L" games in " + 
                        to_wstring(folderCount) + L" folders";
        SendMessageW(hStatusBar, SB_SETTEXTW, 0, (LPARAM)status.c_str());
    }
}

// Filter games by search query
void FilterGames(const wstring& query) {
    if (!hListBox) return;
    
    SendMessageW(hListBox, LB_RESETCONTENT, 0, 0);
    
    wstring lowerQuery = query;
    transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
    
    int matchCount = 0;
    for (const auto& game : games) {
        wstring lowerName = game.name;
        transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        
        if (lowerQuery.empty() || lowerName.find(lowerQuery) != wstring::npos) {
            wstring displayName = game.name;
            if (game.hasMultipleExes) {
                displayName += L" *";
            }
            SendMessageW(hListBox, LB_ADDSTRING, 0, (LPARAM)displayName.c_str());
            matchCount++;
        }
    }
    
    if (hStatusBar) {
        wstring status = L"Showing " + to_wstring(matchCount) + L" of " + 
                        to_wstring(games.size()) + L" games";
        SendMessageW(hStatusBar, SB_SETTEXTW, 0, (LPARAM)status.c_str());
    }
}

// Launch selected game
void LaunchGame(int displayIndex) {
    // Get the actual game name from the listbox
    wchar_t buffer[512];
    SendMessageW(hListBox, LB_GETTEXT, displayIndex, (LPARAM)buffer);
    
    // Remove the " *" suffix if present
    wstring displayName = buffer;
    if (displayName.length() > 2 && displayName.substr(displayName.length() - 2) == L" *") {
        displayName = displayName.substr(0, displayName.length() - 2);
    }
    
    // Find the actual game
    for (const auto& game : games) {
        if (game.name == displayName) {
            // Get the directory for ShellExecute
            wstring exePath = game.exePath;
            size_t lastSlash = exePath.find_last_of(L"\\");
            wstring workingDir = (lastSlash != wstring::npos) ? exePath.substr(0, lastSlash) : L"";
            
            HINSTANCE result = ShellExecuteW(NULL, L"open", game.exePath.c_str(), NULL, 
                                           workingDir.empty() ? NULL : workingDir.c_str(), 
                                           SW_SHOW);
            
            if ((INT_PTR)result <= 32) {
                wstring error = L"Failed to launch: " + game.name + 
                               L"\n\nPath: " + game.exePath +
                               L"\n\nError code: " + to_wstring((INT_PTR)result);
                MessageBoxW(NULL, error.c_str(), L"Launch Error", MB_OK | MB_ICONERROR);
            }
            return;
        }
    }
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            // Create font
            hMainFont = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
            
            // Create search box
            CreateWindowW(L"STATIC", L"Search:",
                WS_CHILD | WS_VISIBLE,
                10, 15, 60, 20,
                hWnd, NULL, NULL, NULL);
            
            hSearchBox = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
                75, 10, 520, 25,
                hWnd, (HMENU)1, NULL, NULL);
            SendMessageW(hSearchBox, WM_SETFONT, (WPARAM)hMainFont, TRUE);
            
            // Create buttons
            HWND hPlayBtn = CreateWindowW(L"BUTTON", L"▶ Play",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                610, 10, 90, 25,
                hWnd, (HMENU)2, NULL, NULL);
            SendMessageW(hPlayBtn, WM_SETFONT, (WPARAM)hMainFont, TRUE);
            
            HWND hRefreshBtn = CreateWindowW(L"BUTTON", L"↻ Refresh",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                710, 10, 90, 25,
                hWnd, (HMENU)3, NULL, NULL);
            SendMessageW(hRefreshBtn, WM_SETFONT, (WPARAM)hMainFont, TRUE);
            
            // Create listbox
            hListBox = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", NULL,
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_HASSTRINGS,
                10, 45, 790, 485,
                hWnd, (HMENU)4, NULL, NULL);
            SendMessageW(hListBox, WM_SETFONT, (WPARAM)hMainFont, TRUE);
            
            // Create status bar
            hStatusBar = CreateWindowExW(0, STATUSCLASSNAMEW, NULL,
                WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
                0, 0, 0, 0,
                hWnd, NULL, NULL, NULL);
            
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
                SetWindowTextW(hSearchBox, L"");
                ScanGames();
            } else if (LOWORD(wParam) == 1 && HIWORD(wParam) == EN_CHANGE) {
                // Search box changed
                wchar_t buffer[256];
                GetWindowTextW(hSearchBox, buffer, 256);
                FilterGames(buffer);
            } else if (LOWORD(wParam) == 4 && HIWORD(wParam) == LBN_DBLCLK) {
                // Double-click on listbox
                int sel = (int)SendMessageW(hListBox, LB_GETCURSEL, 0, 0);
                if (sel != LB_ERR) {
                    LaunchGame(sel);
                }
            }
            return 0;
        }
        
        case WM_SIZE: {
            // Resize status bar
            SendMessageW(hStatusBar, WM_SIZE, 0, 0);
            return 0;
        }
        
        case WM_DESTROY:
            if (hMainFont) DeleteObject(hMainFont);
            PostQuitMessage(0);
            return 0;
    }
    
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);
    
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
        CW_USEDEFAULT, CW_USEDEFAULT, 830, 600,
        NULL, NULL, hInstance, NULL
    );
    
    if (!hWnd) {
        MessageBoxW(NULL, L"Failed to create window!", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    
    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
}
