# VaultPlay Game Launcher

VaultPlay is a Windows-first full-stack desktop game launcher that discovers installed PC games across local drives, enriches the library with useful metadata and artwork, launches the detected executable, tracks playtime, and manages per-game mod preferences from one polished interface.

## Features

- Scans fixed drives, likely game folders, Windows install registry entries, Steam libraries, Epic manifests, and Windows shortcuts.
- Scores executable confidence so the launcher can choose the best playable `.exe` path for each discovered game.
- Enriches matching games with Steam metadata, artwork, release dates, genres, tags, developers, publishers, screenshots, and external links when available.
- Runs an automatic startup scan, manual rescans, return-to-app scans, and a background rescan every 5 minutes.
- Launches games through Electron and tracks total playtime after the launched process exits.
- Shows a cinematic React library with cover flow, hero art, filtering, favorites, recently played game launch cards, and scan proof.
- Always organizes library and recent lists by last played first, then total playtime, then title.
- Provides a large global search box for finding any discovered game by title, drive, tags, path, source, or metadata.
- Builds per-game mod/fix catalogs with concrete purpose rows such as Workshop content, gameplay/texture/QoL mods, total conversions, official subscribed mods, addons/resource packs, BepInEx packs, and PC fix notes.
- Adds per-game `PLAY` buttons in the Mods and Recently Played views so games can be launched with the currently enabled mod profile.
- Persists one-click `Enable` / `Disable` state by game ID and mod ID.
- Filters adapter-missing entries out of the ready mod catalog, so `Adapter required` and `Needs game adapter` rows are not shown as usable mods.

## Accuracy And Safety

No launcher can honestly guarantee every possible game and every possible online mod will always be discovered and applied with zero exceptions. VaultPlay is built to get as close as practical while staying truthful: it scans multiple real Windows sources, verifies executable paths before launching, saves mod enablement permanently, and only displays ready mod/fix entries in the user-facing Mods view.

VaultPlay does not blindly copy mod files into game folders. Provider install pages, official APIs, dependency handling, and per-game rules still matter for real mod installation. The launcher keeps the local enabled/disabled profile and opens the right game with that saved profile state.

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
6. Open `Mods` to enable or disable permanent mod/fix entries per game.
7. Use the per-game `PLAY` buttons in Mods or Recently Played to launch a game with its enabled profile.
8. Click a Recently Played game row to open its full game hub with Play, metadata, and the Permanent Mod Center.

## Project Structure

- `src/`: React renderer, browser preview API bridge, and visual styling.
- `electron/`: Electron main process, preload bridge, game scanner, launcher, mod catalog, and playtime tracking.
- `shared/`: TypeScript types shared between renderer and Electron.
- `scripts/`: Preview API and regression verification scripts.
- `.local-data/`: Local generated library/mod/playtime state, ignored by Git.

## Environment Variables

No `.env` file is required for the current checked-in workflow. Future provider integrations should use environment variables or a local secret store for API keys and OAuth tokens, never hard-coded source values.

## Production Notes

The current project builds the app code but does not yet include an installer packaging script. Before distributing broadly, add a packaging flow, code signing, provider credential setup, and per-game install validation for any automated mod installation feature.
