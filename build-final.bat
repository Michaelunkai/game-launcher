@echo off
setlocal

set PROJECT_DIR=F:\study\Dev_Toolchain\programming\C++\projects\game-launcher
set COMPILER=F:\study\Dev_Toolchain\programming\C++\mingw-complete\mingw64\bin

cd /d "%PROJECT_DIR%"

echo ========================================
echo    Game Launcher - Build Script
echo ========================================
echo.

REM Clean old build
if exist GameLauncher.exe (
    echo Cleaning old build...
    del /Q GameLauncher.exe 2>nul
)
if exist resource.o (
    del /Q resource.o 2>nul
)

REM Step 1: Generate icon (if needed)
if not exist icon.ico (
    echo [1/3] Generating icon...
    powershell -ExecutionPolicy Bypass -File create-icon.ps1
) else (
    echo [1/3] Icon already exists, skipping...
)

REM Step 2: Compile resource
echo [2/3] Compiling resources...
"%COMPILER%\windres.exe" resource.rc -o resource.o
if errorlevel 1 (
    echo ERROR: Resource compilation failed!
    pause
    exit /b 1
)

REM Step 3: Compile launcher
echo [3/3] Compiling launcher...
"%COMPILER%\g++.exe" launcher.cpp resource.o -o GameLauncher.exe -mwindows -municode -lcomctl32 -O2 -s
if errorlevel 1 (
    echo ERROR: Compilation failed!
    pause
    exit /b 1
)

REM Success!
echo.
echo ========================================
echo          BUILD SUCCESSFUL!
echo ========================================
echo.
echo Executable: %PROJECT_DIR%\GameLauncher.exe
dir GameLauncher.exe | findstr "GameLauncher.exe"
echo.
echo To run: GameLauncher.exe
echo.

REM Open explorer to show the file
explorer /select,"%PROJECT_DIR%\GameLauncher.exe"

endlocal
