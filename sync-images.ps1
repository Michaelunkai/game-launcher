# Sync game images from game-library-manager-web repo to cache
param([switch]$Silent)

$srcFolder = "C:\Users\micha\.openclaw\workspace-moltbot\game-library-manager-web\public\images"
$cacheFolder = "F:\study\Dev_Toolchain\programming\C++\projects\game-launcher\cache"
$gamesFolder = "E:\games"

# Create cache folder if needed
if (-not (Test-Path $cacheFolder)) {
    New-Item -ItemType Directory -Path $cacheFolder -Force | Out-Null
}

# Get all game folders
$gameFolders = Get-ChildItem $gamesFolder -Directory -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Name

$copied = 0
$skipped = 0

foreach ($game in $gameFolders) {
    $cacheFile = Join-Path $cacheFolder "$game.jpg"
    
    # Skip if already in cache
    if (Test-Path $cacheFile) {
        $skipped++
        continue
    }
    
    # Normalize game name to lowercase, alphanumeric only
    $searchName = ($game -replace '[^a-zA-Z0-9]', '').ToLower()
    
    # Try exact match
    $srcFile = Join-Path $srcFolder "$searchName.png"
    if (Test-Path $srcFile) {
        Copy-Item $srcFile $cacheFile -Force
        $copied++
        continue
    }
    
    # Try to find file starting with searchName
    $matches = Get-ChildItem $srcFolder -Filter "$searchName*.png" -ErrorAction SilentlyContinue
    if ($matches) {
        Copy-Item $matches[0].FullName $cacheFile -Force
        $copied++
        continue
    }
    
    # Try to find file containing searchName
    $matches = Get-ChildItem $srcFolder -Filter "*$searchName*.png" -ErrorAction SilentlyContinue
    if ($matches) {
        Copy-Item $matches[0].FullName $cacheFile -Force
        $copied++
        continue
    }
}

if (-not $Silent) {
    Write-Host "Synced: $copied new images, $skipped already cached"
}
