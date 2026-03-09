@echo off
cd /d "%~dp0"

:: Sync images from game-library-manager-web repo (silent mode)
powershell -ExecutionPolicy Bypass -File "sync-images.ps1" -Silent

:: Start the game launcher
start "" "GameLauncher.exe"
