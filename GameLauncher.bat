@echo off
cd /d "%~dp0"

REM Sync images from game-library-manager-web repo (silent mode)
powershell -ExecutionPolicy Bypass -File "sync-images.ps1" -Silent

REM Start the game launcher
start "" "GameLauncher.exe"
