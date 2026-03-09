@echo off
cd /d "F:\study\Dev_Toolchain\programming\C++\projects\game-launcher"
echo Compiling...
C:\ProgramData\mingw64\mingw64\bin\g++.exe test.cpp -o test.exe -mwindows -municode > compile_log.txt 2>&1
type compile_log.txt
if exist test.exe (
    echo.
    echo SUCCESS!
) else (
    echo.
    echo FAILED - see compile_log.txt for details
)
