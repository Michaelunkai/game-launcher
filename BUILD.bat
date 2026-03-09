@echo off
setlocal
title Building Game Launcher...

:: Set MinGW path
set PATH=C:\ProgramData\mingw64\mingw64\bin;%PATH%

cd /d "F:\study\Dev_Toolchain\programming\C++\projects\game-launcher"

echo.
echo ========================================
echo   GAME LAUNCHER - ULTRA PREMIUM BUILD
echo ========================================
echo.

:: Compile resource
echo [1/2] Compiling resources...
windres resource.rc -o resource.o 2>nul

:: Compile main application
echo [2/2] Building application...
g++ -std=c++17 -O2 -DNDEBUG -mwindows -municode ^
    launcher-ultra.cpp ^
    resource.o ^
    -o GameLauncher.exe ^
    -lcomctl32 -lgdiplus -ldwmapi -luxtheme -lshlwapi -lwinhttp ^
    -static-libgcc -static-libstdc++

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo   BUILD SUCCESSFUL!
    echo ========================================
    echo   Output: GameLauncher.exe
    echo   Size: 
    for %%F in (GameLauncher.exe) do echo          %%~zF bytes
    echo ========================================
    echo.
    echo Starting application...
    start "" "GameLauncher.exe"
) else (
    echo.
    echo ========================================
    echo   BUILD FAILED!
    echo ========================================
    echo   Check errors above.
    echo ========================================
    pause
)
