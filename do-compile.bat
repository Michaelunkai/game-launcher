@echo off
cd /d "F:\study\Dev_Toolchain\programming\C++\projects\game-launcher"
echo Compiling simple-launcher.cpp...
echo.

C:\ProgramData\mingw64\mingw64\bin\g++.exe -o GameLauncher.exe simple-launcher.cpp -mwindows -municode -O2 -s >log.txt 2>&1

echo.
if exist GameLauncher.exe (
    echo SUCCESS - GameLauncher.exe created!
    dir GameLauncher.exe
) else (
    echo FAILED!
    echo Check log.txt for errors
    type log.txt
)
