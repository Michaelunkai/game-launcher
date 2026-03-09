# Game Launcher Pro

A Windows game launcher that automatically scans `E:\games` and displays games with cover images from the game-library-manager-web repository.

## Features

- **Auto-sync images** - At startup and when clicking Refresh, images are synced from `game-library-manager-web/public/images` (1036+ game covers)
- **Smart image matching** - Matches game folder names to image files even with different casing or naming conventions
- **Search** - Filter games by typing in the search box
- **Preview** - Click a game to see its cover image
- **Launch** - Double-click or press Play to launch the game

## Files

- `GameLauncher.exe` - The main application (statically linked, no DLLs needed)
- `GameLauncher.bat` - Launcher that syncs images before starting the app
- `sync-images.ps1` - PowerShell script that syncs images from the repo
- `launcher-with-images.cpp` - Source code
- `cache/` - Cached game images

## How It Works

1. **Startup**: The app runs `sync-images.ps1` to copy any missing game images from `C:\Users\micha\.openclaw\workspace-moltbot\game-library-manager-web\public\images` to the local cache
2. **Scanning**: Scans `E:\games` for subfolders containing `.exe` files
3. **Images**: Looks for cached images matching the game folder name
4. **Display**: Shows games in a list with their cover images

## Adding New Games

1. Add your game folder to `E:\games`
2. Make sure your game-library-manager-web repo has an image for the game (in `public/images/`)
3. Click Refresh in the app (or restart it)
4. The image will automatically appear

## Building

```batch
F:\study\Dev_Toolchain\programming\C++\mingw-complete\mingw64\bin\g++.exe launcher-with-images.cpp resource.o -o GameLauncher.exe -mwindows -municode -lcomctl32 -lwinhttp -lgdiplus -lshlwapi -O2 -s -static
```

## Dependencies

- MinGW-w64 (for compilation)
- PowerShell (for sync script)
- game-library-manager-web repo (for images)

## Image Sources

Images are synced from: `C:\Users\micha\.openclaw\workspace-moltbot\game-library-manager-web\public\images`

This repo contains 1036+ game cover images.
