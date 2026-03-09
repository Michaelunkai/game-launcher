@echo off
setlocal

:: Set paths
set MINGW=C:\msys64\mingw64\bin
set PATH=%MINGW%;%PATH%

cd /d "F:\study\Dev_Toolchain\programming\C++\projects\game-launcher"

echo Building Premium Game Launcher...

:: Compile resource
windres resource.rc -o resource.o 2>nul

:: Compile main application
g++ -std=c++17 -O2 -DNDEBUG -mwindows ^
    launcher-premium.cpp ^
    resource.o ^
    -o GameLauncher.exe ^
    -lcomctl32 -lgdiplus -ldwmapi -luxtheme -lshlwapi -lwinhttp ^
    -static-libgcc -static-libstdc++

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo BUILD SUCCESSFUL!
    echo ========================================
    echo Output: GameLauncher.exe
) else (
    echo.
    echo BUILD FAILED!
    echo Check compilation errors above.
)

pause
