# Build script for Game Launcher
$ErrorActionPreference = "Continue"

$projectDir = "F:\study\Dev_Toolchain\programming\C++\projects\game-launcher"
Set-Location $projectDir

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Building Game Launcher..." -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# Step 1: Create icon
Write-Host "`n[1/4] Creating icon..." -ForegroundColor Yellow
powershell -ExecutionPolicy Bypass -File "create-icon.ps1"

if (-not (Test-Path "icon.ico")) {
    Write-Host "ERROR: Icon creation failed!" -ForegroundColor Red
    exit 1
}

# Step 2: Compile resource file
Write-Host "`n[2/4] Compiling resource file..." -ForegroundColor Yellow

# Try windres (MinGW)
if (Get-Command windres -ErrorAction SilentlyContinue) {
    windres resource.rc -o resource.o
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Resource compiled successfully with windres" -ForegroundColor Green
        $useWindres = $true
    }
} else {
    Write-Host "windres not found, trying rc.exe (MSVC)..." -ForegroundColor Yellow
    
    # Try to find rc.exe (Windows SDK)
    $rcPaths = @(
        "C:\Program Files (x86)\Windows Kits\10\bin\*\x64\rc.exe",
        "C:\Program Files (x86)\Windows Kits\10\bin\*\x86\rc.exe",
        "C:\Program Files\Microsoft SDKs\Windows\*\bin\rc.exe"
    )
    
    $rcExe = $null
    foreach ($path in $rcPaths) {
        $found = Get-Item $path -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($found) {
            $rcExe = $found.FullName
            break
        }
    }
    
    if ($rcExe) {
        & $rcExe /fo resource.res resource.rc
        if ($LASTEXITCODE -eq 0) {
            Write-Host "Resource compiled successfully with rc.exe" -ForegroundColor Green
            $useWindres = $false
        }
    } else {
        Write-Host "WARNING: No resource compiler found, building without icon..." -ForegroundColor Yellow
        $useWindres = $null
    }
}

# Step 3: Compile C++ code
Write-Host "`n[3/4] Compiling C++ code..." -ForegroundColor Yellow

# Try g++ (MinGW)
if (Get-Command g++ -ErrorAction SilentlyContinue) {
    Write-Host "Using g++ (MinGW)..." -ForegroundColor Cyan
    
    $compileCmd = "g++ -o GameLauncher.exe main.cpp"
    
    if ($useWindres) {
        $compileCmd += " resource.o"
    }
    
    $compileCmd += " -mwindows -municode -lgdiplus -lcomctl32 -lshlwapi -lwininet -O2 -s"
    
    Write-Host "Command: $compileCmd" -ForegroundColor Gray
    Invoke-Expression $compileCmd
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Compilation successful!" -ForegroundColor Green
    } else {
        Write-Host "ERROR: Compilation failed!" -ForegroundColor Red
        exit 1
    }
}
# Try cl (MSVC)
elseif (Get-Command cl -ErrorAction SilentlyContinue) {
    Write-Host "Using cl (MSVC)..." -ForegroundColor Cyan
    
    $compileCmd = "cl /EHsc /O2 /Fe:GameLauncher.exe main.cpp gdiplus.lib comctl32.lib shlwapi.lib wininet.lib user32.lib gdi32.lib /link /SUBSYSTEM:WINDOWS"
    
    if (-not $useWindres -and (Test-Path "resource.res")) {
        $compileCmd += " resource.res"
    }
    
    Write-Host "Command: $compileCmd" -ForegroundColor Gray
    Invoke-Expression $compileCmd
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Compilation successful!" -ForegroundColor Green
    } else {
        Write-Host "ERROR: Compilation failed!" -ForegroundColor Red
        exit 1
    }
}
else {
    Write-Host "ERROR: No C++ compiler found!" -ForegroundColor Red
    Write-Host "Please install MinGW-w64 or Visual Studio Build Tools" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "To install MinGW-w64:" -ForegroundColor Cyan
    Write-Host "  winget install -e --id=MSYS2.MSYS2" -ForegroundColor White
    Write-Host "  Then run in MSYS2: pacman -S mingw-w64-x86_64-gcc" -ForegroundColor White
    Write-Host "  Add to PATH: C:\msys64\mingw64\bin" -ForegroundColor White
    exit 1
}

# Step 4: Verify output
Write-Host "`n[4/4] Verifying build..." -ForegroundColor Yellow

if (Test-Path "GameLauncher.exe") {
    $fileInfo = Get-Item "GameLauncher.exe"
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Green
    Write-Host "[SUCCESS] BUILD COMPLETE!" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "Executable: $($fileInfo.FullName)" -ForegroundColor Cyan
    Write-Host "Size: $([math]::Round($fileInfo.Length / 1KB, 2)) KB" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "To run: .\GameLauncher.exe" -ForegroundColor Yellow
    Write-Host ""
    
    # Clean up build artifacts
    Remove-Item *.o, *.res, *.obj -ErrorAction SilentlyContinue
} else {
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Red
    Write-Host "[ERROR] BUILD FAILED!" -ForegroundColor Red
    Write-Host "========================================" -ForegroundColor Red
    exit 1
}
