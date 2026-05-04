# VaultPlay Game Launcher

VaultPlay is a Windows-first full-stack desktop game launcher that discovers installed PC games across local drives, enriches the library with useful metadata and artwork, launches the detected executable, tracks playtime, and manages per-game mod preferences from one polished interface.

## Features

- Scans fixed drives, likely game folders, Windows install registry entries, Steam libraries, Epic manifests, and Windows shortcuts.
- Scores executable confidence so the launcher can choose the best playable `.exe` path for each discovered game.
- Enriches matching games with Steam metadata, artwork, release dates, genres, tags, developers, publishers, screenshots, and external links when available.
- Runs an automatic startup scan, manual rescans, return-to-app scans, and a background rescan every 5 minutes.
- Launches games through Electron and tracks total playtime after the launched process exits.
- Shows a cinematic React library with cover flow, hero art, filtering, sorting, favorites, recently played games, and scan proof.
- Builds per-game mod catalogs with trainer-style entries, provider searches, Steam Workshop, Nexus Mods, ModDB, mod.io, CurseForge, Thunderstore, and PCGamingWiki routes.
- Persists mod preferences by game ID and mod ID, while separating saved preferences from verified provider/API/adapter integration status.
- Refuses to claim adapter-required mods are working until a provider route, API connection, or game-specific adapter can validate safe application.

## Accuracy And Safety

No launcher can honestly guarantee every possible game and every possible online mod will always be discovered and applied with zero exceptions. VaultPlay is built to get as close as practical while staying truthful: it scans multiple real Windows sources, verifies executable paths before launching, saves mod preferences permanently, and clearly labels whether a mod source is provider-managed, API-connectable, reference-only, or still needs a game-specific adapter.

VaultPlay does not blindly copy mod files into game folders. One-click real mod installation requires provider credentials, per-game install rules, dependency handling, and explicit adapter support.

## Tech Stack

- Electron for the Windows desktop shell and native process launching.
- React 19 and Vite for the renderer UI.
- TypeScript for shared contracts, renderer code, Electron main process code, and scanner logic.
- Node.js APIs for filesystem, process, shortcut, registry, and local persistence work.

## Requirements

- Windows 11 is the primary target.
- Node.js compatible with the checked-in lockfile and scripts.
- npm.
- Internet access is optional but recommended for metadata enrichment and provider links.

The browser preview can run on macOS or Linux for UI development, but real drive scanning, Windows shortcuts, registry discovery, and executable launching are Windows-focused.

## Install

```powershell
npm install
```

## Development

Run the full Electron app:

```powershell
npm run dev
```

Run the browser preview and local preview API separately:

```powershell
npm run preview
npm run preview:api
```

Run both preview servers together:

```powershell
npm run preview:full
```

Default local URLs:

- Renderer preview: `http://127.0.0.1:4173/`
- Preview API: `http://127.0.0.1:5274/api`

## Build

```powershell
npm run build
```

The build writes renderer output to `dist/` and Electron TypeScript output to `dist-electron/`. Both are generated artifacts and are ignored by Git.

## Verification

Run type checks:

```powershell
npm run typecheck
```

Run the production build:

```powershell
npm run build
```

Verify backend library, launch validation, mod catalog integrity, and truthful mod status:

```powershell
npm run verify:backend
```

Verify mod preference persistence and restoration:

```powershell
npm run verify:mods
```

Run the full real scanner regression:

```powershell
npm run verify:scanner
```

Run a dependency audit:

```powershell
npm audit --audit-level=high
```

There is no lint script in this repository yet, so `npm run lint` is intentionally not documented as an available command.

## How To Use

1. Install dependencies with `npm install`.
2. Start the app with `npm run dev`, or use `npm run preview:full` for browser/API preview.
3. Let the startup scan complete, or press `Auto-scan all drives`.
4. Pick a game from the library cover flow or list.
5. Press `Play` to launch the detected executable and track playtime.
6. Open `Mods` to save or remove permanent mod preferences per game.
7. Treat `Needs game adapter` entries as saved desired mods, not guaranteed in-game activation.

## Project Structure

- `src/`: React renderer, browser preview API bridge, and visual styling.
- `electron/`: Electron main process, preload bridge, game scanner, launcher, mod catalog, and playtime tracking.
- `shared/`: TypeScript types shared between renderer and Electron.
- `scripts/`: Preview API and regression verification scripts.
- `.local-data/`: Local generated library/mod/playtime state, ignored by Git.

## Environment Variables

No `.env` file is required for the current checked-in workflow. Future provider integrations should use environment variables or a local secret store for API keys and OAuth tokens, never hard-coded source values.

## Production Notes

The current project builds the app code but does not yet include an installer packaging script. Before distributing broadly, add a packaging flow, code signing, provider credential setup, and per-game mod adapter validation for any automated mod installation feature.
