@echo off
cd /d "F:\study\Dev_Toolchain\programming\C++\projects\game-launcher"
echo Compiling Game Launcher...
C:\ProgramData\mingw64\mingw64\bin\g++.exe -o GameLauncher.exe main.cpp -mwindows -municode -lgdiplus -lcomctl32 -lshlwapi -lwininet -O2 -s 2>&1
if exist GameLauncher.exe (
    echo SUCCESS! GameLauncher.exe created.
    dir GameLauncher.exe
) else (
    echo FAILED! Check errors above.
)
pause
