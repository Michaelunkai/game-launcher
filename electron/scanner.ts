import { execFile } from "node:child_process";
import { promises as fs } from "node:fs";
import os from "node:os";
import path from "node:path";
import { promisify } from "node:util";
import type { GameRecord, GameSource, MatchStatus, ScanProgress, ScanResult } from "../shared/types.js";

const execFileAsync = promisify(execFile);

const GAME_HINTS = [
  "steamapps",
  "steamlibrary",
  "epic games",
  "gog galaxy",
  "gog games",
  "ubisoft",
  "ea games",
  "electronic arts",
  "battle.net",
  "battlenet",
  "xboxgames",
  "games",
  "game-library",
  "pc games",
];

const NON_GAME_EXES = [
  "unins",
  "setup",
  "install",
  "redist",
  "vcredist",
  "crash",
  "quicksfv",
  "sfv",
  "launcherhelper",
  "unitycrashhandler",
  "dxsetup",
  "ue4prereq",
  "dotnet",
  "directx",
  "eac",
  "easyanticheat",
  "benchmark",
  "config",
  "settings",
  "helper",
  "service",
  "updater",
  "update",
];

const GAME_PUBLISHER_HINTS = [
  "activision",
  "bandai",
  "bethesda",
  "blizzard",
  "capcom",
  "devolver",
  "electronic arts",
  "epic games",
  "focus",
  "gog",
  "konami",
  "microsoft studios",
  "nacon",
  "namco",
  "paradox",
  "riot games",
  "rockstar",
  "sega",
  "sony",
  "square enix",
  "steam",
  "take-two",
  "ubisoft",
  "valve",
  "warner",
  "xbox",
];

type SteamAppListItem = {
  appid: number;
  name: string;
};

type SteamDetails = {
  type?: string;
  name?: string;
  steam_appid?: number;
  required_age?: number | string;
  is_free?: boolean;
  detailed_description?: string;
  short_description?: string;
  supported_languages?: string;
  header_image?: string;
  capsule_image?: string;
  capsule_imagev5?: string;
  website?: string;
  developers?: string[];
  publishers?: string[];
  platforms?: Record<string, boolean>;
  categories?: Array<{ id?: number; description?: string }>;
  genres?: Array<{ id?: string; description?: string }>;
  screenshots?: Array<{ id?: number; path_thumbnail?: string; path_full?: string }>;
  release_date?: { coming_soon?: boolean; date?: string };
};

type SteamSearchItem = {
  id?: number;
  name?: string;
  type?: string;
};

let steamAppListCache: SteamAppListItem[] | undefined;

export async function scanInstalledGames(): Promise<ScanResult> {
  const progress: ScanProgress[] = [];
  const scannedDrives = await getFixedDrives();
  const discovered = new Map<string, GameRecord>();
  const discoveryDeadline = Date.now() + 75_000;

  await addSteamGames(discovered, progress);
  await addEpicGames(discovered, progress);
  await addRegistryGames(discovered, progress, discoveryDeadline);
  await addGameLibraryFolders(scannedDrives, discovered, progress, discoveryDeadline);
  await addHeuristicExecutables(scannedDrives, discovered, progress, discoveryDeadline);
  await addShortcutGames(discovered, progress, discoveryDeadline);

  const enriched = await enrichGames([...discovered.values()], progress, Date.now() + 75_000);
  const games = enriched.sort((a, b) => a.title.localeCompare(b.title));

  return {
    games,
    progress,
    scannedDrives,
    completedAt: new Date().toISOString(),
  };
}

async function getFixedDrives(): Promise<string[]> {
  try {
    const { stdout } = await execFileAsync("powershell.exe", [
      "-NoProfile",
      "-Command",
      "Get-CimInstance Win32_LogicalDisk -Filter \"DriveType=3\" | Select-Object -ExpandProperty DeviceID",
    ], { timeout: 4_000 });
    return stdout
      .split(/\r?\n/)
      .map((drive) => drive.trim())
      .filter(Boolean)
      .map((drive) => `${drive}\\`);
  } catch {
    return ["C:\\"];
  }
}

async function addSteamGames(discovered: Map<string, GameRecord>, progress: ScanProgress[]) {
  const candidates = [
    path.join(process.env["ProgramFiles(x86)"] ?? "C:\\Program Files (x86)", "Steam", "steamapps", "libraryfolders.vdf"),
    path.join(process.env.ProgramFiles ?? "C:\\Program Files", "Steam", "steamapps", "libraryfolders.vdf"),
    ...((await getFixedDrives()).flatMap((drive) => [
      path.join(drive, "Steam", "steamapps", "libraryfolders.vdf"),
      path.join(drive, "SteamLibrary", "steamapps", "libraryfolders.vdf"),
      path.join(drive, "Games", "Steam", "steamapps", "libraryfolders.vdf"),
      path.join(drive, "Games", "SteamLibrary", "steamapps", "libraryfolders.vdf"),
    ])),
  ];

  for (const libraryFile of candidates) {
    if (!(await exists(libraryFile))) continue;
    const content = await fs.readFile(libraryFile, "utf8");
    const libraries = extractQuotedPaths(content).filter((entry) => /:\\\\|:\//.test(entry));
    libraries.push(path.dirname(path.dirname(libraryFile)));

    for (const library of unique(libraries)) {
      const steamApps = path.join(library.replaceAll("\\\\", "\\"), "steamapps");
      if (!(await exists(steamApps))) continue;
      const manifests = await safeReaddir(steamApps);
      for (const manifest of manifests.filter((name) => /^appmanifest_\d+\.acf$/i.test(name))) {
        const manifestPath = path.join(steamApps, manifest);
        const manifestContent = await fs.readFile(manifestPath, "utf8");
        const title = extractVdfValue(manifestContent, "name") ?? titleFromPath(manifest);
        const installDir = extractVdfValue(manifestContent, "installdir") ?? title;
        const installPath = path.join(steamApps, "common", installDir);
        const exe = await bestExecutableIn(installPath);
        const appId = /^appmanifest_(\d+)\.acf$/i.exec(manifest)?.[1];
        if (exe) addGame(discovered, await createGame(title, exe, installPath, "steam", "verified", 94, 88, { steamAppId: appId }));
      }
    }
  }

  progress.push({ phase: "Steam libraries checked", checked: unique(candidates).length, found: discovered.size });
}

async function addEpicGames(discovered: Map<string, GameRecord>, progress: ScanProgress[]) {
  const manifestDir = path.join(process.env.ProgramData ?? "C:\\ProgramData", "Epic", "EpicGamesLauncher", "Data", "Manifests");
  const manifests = await safeReaddir(manifestDir);

  for (const manifest of manifests.filter((name) => name.endsWith(".item"))) {
    try {
      const parsed = JSON.parse(await fs.readFile(path.join(manifestDir, manifest), "utf8")) as {
        DisplayName?: string;
        InstallLocation?: string;
        LaunchExecutable?: string;
        CatalogItemId?: string;
      };
      const title = parsed.DisplayName?.trim();
      const installPath = parsed.InstallLocation?.trim();
      if (!title || !installPath) continue;
      const exe = parsed.LaunchExecutable
        ? path.join(installPath, parsed.LaunchExecutable)
        : await bestExecutableIn(installPath);
      if (exe && (await exists(exe))) {
        addGame(
          discovered,
          await createGame(title, exe, installPath, "epic", "verified", 96, 86, {
            epicCatalogItemId: parsed.CatalogItemId,
          }),
        );
      }
    } catch {
      continue;
    }
  }

  progress.push({ phase: "Epic manifests checked", checked: manifests.length, found: discovered.size });
}

async function addRegistryGames(discovered: Map<string, GameRecord>, progress: ScanProgress[], deadline: number) {
  const rows = await readUninstallRegistry(deadline);
  let checked = 0;

  for (const row of rows) {
    if (Date.now() > deadline) break;
    const title = row.DisplayName?.trim();
    const installPath = normalizeRegistryPath(row.InstallLocation);
    const displayIcon = normalizeDisplayIcon(row.DisplayIcon);
    if (!title || !isLikelyRegistryGame(row, installPath, displayIcon)) continue;
    checked += 1;

    const exe = displayIcon && displayIcon.toLowerCase().endsWith(".exe") && (await exists(displayIcon))
      ? displayIcon
      : installPath
        ? await bestExecutableIn(installPath)
        : undefined;

    if (!exe || isNonGameExe(exe)) continue;
    const root = installPath && (await exists(installPath)) ? installPath : inferGameRoot(exe);
    addGame(discovered, await createGame(bestTitleFromFolderAndExe(title, exe), exe, root, "registry", "likely", 84, 70));
  }

  progress.push({ phase: "Windows install registry checked", checked, found: discovered.size });
}

async function readUninstallRegistry(deadline: number): Promise<Array<Record<string, string | undefined>>> {
  if (Date.now() > deadline) return [];
  try {
    const command = [
      "$paths=@(",
      "'HKLM:\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\*',",
      "'HKLM:\\Software\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\*',",
      "'HKCU:\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\*'",
      ");",
      "Get-ItemProperty $paths -ErrorAction SilentlyContinue |",
      "Where-Object { $_.DisplayName } |",
      "Select-Object DisplayName,DisplayVersion,Publisher,InstallLocation,DisplayIcon,UninstallString |",
      "ConvertTo-Json -Compress",
    ].join(" ");
    const { stdout } = await execFileAsync("powershell.exe", ["-NoProfile", "-Command", command], { timeout: 8_000 });
    if (!stdout.trim()) return [];
    const parsed = JSON.parse(stdout) as Record<string, string | undefined> | Array<Record<string, string | undefined>>;
    return Array.isArray(parsed) ? parsed : [parsed];
  } catch {
    return [];
  }
}

async function addShortcutGames(discovered: Map<string, GameRecord>, progress: ScanProgress[], deadline: number) {
  const desktopPaths = [
    path.join(process.env.USERPROFILE ?? "", "Desktop"),
    path.join(process.env.PUBLIC ?? "C:\\Users\\Public", "Desktop"),
    path.join(process.env.ProgramData ?? "C:\\ProgramData", "Microsoft", "Windows", "Start Menu", "Programs"),
  ].filter(Boolean);

  let checked = 0;
  for (const root of desktopPaths) {
    if (Date.now() > deadline) break;
    const shortcuts = await collectFiles(root, [".lnk"], 3, 140, Math.min(deadline, Date.now() + 5_000));
    checked += shortcuts.length;
    for (const shortcut of shortcuts) {
      if (Date.now() > deadline) break;
      const target = await resolveShortcut(shortcut);
      if (!target || !target.toLowerCase().endsWith(".exe") || isNonGameExe(target)) continue;
      if (!isLikelyGamePath(target) && !isLikelyGamePath(shortcut)) continue;
      const title = titleFromPath(shortcut.replace(/\.lnk$/i, ""));
      addGame(discovered, await createGame(title, target, path.dirname(target), "windows-shortcut", "likely", 82, 68));
    }
  }

  progress.push({ phase: "Windows shortcuts resolved", checked, found: discovered.size });
}

async function addGameLibraryFolders(
  drives: string[],
  discovered: Map<string, GameRecord>,
  progress: ScanProgress[],
  deadline: number,
) {
  let checked = 0;
  for (const drive of drives) {
    if (Date.now() > deadline) break;
    const libraryRoots = [
      path.join(drive, "Games"),
      path.join(drive, "games"),
      path.join(drive, "GameLibrary"),
      path.join(drive, "game-library"),
      path.join(drive, "PC Games"),
      path.join(drive, "SteamLibrary", "steamapps", "common"),
      path.join(drive, "XboxGames"),
      path.join(drive, "GOG Games"),
      path.join(drive, "Program Files", "Epic Games"),
      path.join(drive, "Program Files", "GOG Galaxy", "Games"),
      path.join(drive, "Program Files", "EA Games"),
      path.join(drive, "Program Files", "Ubisoft", "Ubisoft Game Launcher", "games"),
      path.join(drive, "Program Files (x86)", "Steam", "steamapps", "common"),
      path.join(drive, "Program Files (x86)", "Ubisoft", "Ubisoft Game Launcher", "games"),
    ];

    for (const root of libraryRoots) {
      if (Date.now() > deadline || !(await exists(root))) continue;
      const children = await safeDirents(root);
      for (const child of children) {
        if (Date.now() > deadline) break;
        if (!child.isDirectory() || shouldSkipGameFolder(child.name)) continue;
        const childPath = path.join(root, child.name);
        if (isContainerGameFolder(child.name)) {
          const nestedChildren = await safeDirents(childPath);
          for (const nested of nestedChildren) {
            if (Date.now() > deadline) break;
            if (!nested.isDirectory() || shouldSkipGameFolder(nested.name)) continue;
            checked += 1;
            await addFolderCandidate(discovered, path.join(childPath, nested.name), nested.name);
          }
        } else {
          checked += 1;
          await addFolderCandidate(discovered, childPath, child.name);
        }
      }
    }
  }

  progress.push({ phase: "Game library folders checked", checked, found: discovered.size });
}

async function addFolderCandidate(discovered: Map<string, GameRecord>, installPath: string, folderName: string) {
  const exe = await bestExecutableIn(installPath);
  if (exe) {
    addGame(discovered, await createGame(bestTitleFromFolderAndExe(folderName, exe), exe, installPath, "heuristic-exe", "likely", 78, 62));
  }
}

async function addHeuristicExecutables(
  drives: string[],
  discovered: Map<string, GameRecord>,
  progress: ScanProgress[],
  deadline: number,
) {
  let checked = 0;
  for (const drive of drives) {
    if (Date.now() > deadline) break;
    const roots = await safeReaddir(drive);
    const likelyRoots = roots
      .map((entry) => path.join(drive, entry))
      .filter((entry) => GAME_HINTS.some((hint) => entry.toLowerCase().includes(hint)));

    for (const root of likelyRoots) {
      if (Date.now() > deadline) break;
      const exes = await collectFiles(root, [".exe"], 4, 1_200, deadline);
      checked += exes.length;
      for (const exe of exes.filter((candidate) => !isNonGameExe(candidate))) {
        const gameRoot = inferGameRoot(exe);
        if (shouldSkipGameFolder(path.basename(gameRoot)) || isContainerGameFolder(path.basename(gameRoot))) continue;
        const title = bestTitleFromFolderAndExe(titleFromPath(gameRoot), exe);
        addGame(discovered, await createGame(title, exe, gameRoot, "heuristic-exe", "needs-review", 62, 54));
      }
    }
  }

  progress.push({ phase: "Drive heuristics checked", checked, found: discovered.size });
}

async function resolveShortcut(shortcutPath: string): Promise<string | undefined> {
  try {
    const escaped = shortcutPath.replaceAll("'", "''");
    const { stdout } = await execFileAsync("powershell.exe", [
      "-NoProfile",
      "-Command",
      `$s=(New-Object -ComObject WScript.Shell).CreateShortcut('${escaped}'); $s.TargetPath`,
    ], { timeout: 1_200 });
    return stdout.trim() || undefined;
  } catch {
    return undefined;
  }
}

async function bestExecutableIn(root: string): Promise<string | undefined> {
  const exes = await collectFiles(root, [".exe"], 5, 1_400, Date.now() + 5_500);
  return exes
    .filter((exe) => !isNonGameExe(exe))
    .sort((a, b) => scoreExecutable(b, root) - scoreExecutable(a, root))[0];
}

function scoreExecutable(exe: string, root: string): number {
  const lower = exe.toLowerCase();
  const base = path.basename(exe, ".exe").toLowerCase();
  const rootName = path.basename(root).toLowerCase();
  let score = 0;
  if (base.includes(rootName) || rootName.includes(base)) score += 30;
  if (lower.startsWith(root.toLowerCase())) score += 8;
  if (lower.includes("\\binaries\\") && lower.includes("\\win64\\")) score += 14;
  if (!lower.includes("\\engine\\")) score += 8;
  if (lower.includes("\\win64\\") || lower.includes("\\x64\\")) score += 10;
  if (base.includes("shipping")) score += 8;
  if (base.includes("launcher")) score -= 12;
  if (base === "launcher") score -= 15;
  if (base === "game" || base === "client") score -= 6;
  if (lower.includes("\\tools\\") || lower.includes("\\extras\\")) score -= 22;
  return score;
}

async function collectFiles(
  root: string,
  extensions: string[],
  maxDepth: number,
  maxFiles: number,
  deadline: number = Date.now() + 8_000,
): Promise<string[]> {
  const results: string[] = [];
  async function walk(current: string, depth: number) {
    if (Date.now() > deadline || results.length >= maxFiles || depth > maxDepth) return;
    const entries = await safeDirents(current);
    for (const entry of entries) {
      if (Date.now() > deadline) return;
      const fullPath = path.join(current, entry.name);
      if (entry.isDirectory()) {
        if (!shouldSkipDirectory(entry.name)) await walk(fullPath, depth + 1);
      } else if (extensions.includes(path.extname(entry.name).toLowerCase())) {
        results.push(fullPath);
      }
      if (results.length >= maxFiles) return;
    }
  }
  await walk(root, 0);
  return results;
}

async function createGame(
  title: string,
  executablePath: string,
  installPath: string,
  source: GameSource,
  matchStatus: MatchStatus,
  executableConfidence: number,
  metadataConfidence: number,
  externalIds: GameRecord["externalIds"] = {},
): Promise<GameRecord> {
  const normalizedTitle = cleanTitle(title);
  const seed = hash(`${normalizedTitle}:${executablePath}`);
  const stats = await safeStat(executablePath);
  const localSteamAppId = externalIds?.steamAppId ?? (await findSteamAppId(installPath, executablePath));
  const resolvedExternalIds = { ...externalIds, ...(localSteamAppId ? { steamAppId: localSteamAppId } : {}) };
  const steamAppId = resolvedExternalIds.steamAppId;
  const steamImages = steamAppId
    ? {
        hero: `https://cdn.akamai.steamstatic.com/steam/apps/${steamAppId}/library_hero.jpg`,
        cover: `https://cdn.akamai.steamstatic.com/steam/apps/${steamAppId}/library_600x900.jpg`,
      }
    : undefined;
  return {
    id: seed,
    title: normalizedTitle,
    executablePath,
    installPath,
    drive: path.parse(installPath).root,
    source,
    externalIds: resolvedExternalIds,
    links: {
      steamStore: steamAppId ? `https://store.steampowered.com/app/${steamAppId}` : undefined,
      pcGamingWiki: `https://www.pcgamingwiki.com/w/index.php?search=${encodeURIComponent(normalizedTitle)}`,
      steamWorkshop: steamAppId ? `https://steamcommunity.com/app/${steamAppId}/workshop/` : undefined,
      nexusModsSearch: `https://www.nexusmods.com/search/?gsearch=${encodeURIComponent(normalizedTitle)}`,
      modDbSearch: `https://www.moddb.com/search?q=${encodeURIComponent(normalizedTitle)}`,
    },
    dataSources: [
      "Windows filesystem",
      sourceLabel(source),
      ...(steamAppId ? ["Steam app id file", "Steam CDN artwork"] : []),
      "Local executable metadata",
    ],
    matchStatus,
    executableConfidence,
    metadataConfidence: steamAppId ? Math.max(metadataConfidence, 92) : metadataConfidence,
    totalPlaySeconds: 0,
    firstDiscoveredAt: new Date().toISOString(),
    heroImage: steamImages?.hero ?? gradient(seed, true),
    coverImage: steamImages?.cover ?? gradient(seed, false),
    genres: inferGenres(normalizedTitle),
    tags: [
      "Real local scan",
      sourceLabel(source),
      matchStatus === "verified" ? "Executable verified" : "Reviewable match",
      ...(steamAppId ? ["Steam artwork"] : []),
    ],
    developers: [],
    publishers: [],
    releaseDate: undefined,
    screenshots: [],
    platforms: ["Windows"],
    controllerSupport: executableConfidence > 80,
    cloudSave: source === "steam" || source === "epic",
    modSupport: source === "steam" ? "community" : "unknown",
    description: steamAppId
      ? "Discovered from a real Steam app manifest with executable selection, Steam artwork URLs, local install path, and playtime tracking ready."
      : "Discovered from real Windows sources with executable path, install location, file metadata, confidence scoring, and playtime tracking ready.",
    technicalDetails: {
      executableName: path.basename(executablePath),
      executableSizeMb: stats ? Math.round((stats.size / 1024 / 1024) * 10) / 10 : undefined,
      lastModifiedAt: stats?.mtime.toISOString(),
      metadataStatus: steamAppId ? "Steam metadata and images linked automatically" : "Local metadata extracted; online match provider not configured yet",
      discoveryProof: [
        `Source: ${sourceLabel(source)}`,
        `Install path: ${installPath}`,
        `Executable exists: ${stats ? "yes" : "unknown"}`,
        `Executable score: ${executableConfidence}%`,
      ],
    },
  };
}

async function enrichGames(games: GameRecord[], progress: ScanProgress[], deadline: number): Promise<GameRecord[]> {
  if (!games.length || Date.now() > deadline) {
    progress.push({ phase: "Online metadata enriched", checked: 0, found: 0 });
    return games;
  }

  const appList = await loadSteamAppList(Math.min(deadline, Date.now() + 4_000));
  const candidates = games.map((game) => ({
    game,
    steamAppId: game.externalIds?.steamAppId ?? matchSteamAppId(game, appList),
  }));
  const maxToFetch = Math.max(80, candidates.length);
  let enrichedCount = 0;
  let checked = 0;

  const enriched = await mapLimit(candidates, 5, async ({ game, steamAppId }) => {
    if (checked >= maxToFetch || Date.now() > deadline) return game;
    checked += 1;
    const resolvedSteamAppId = steamAppId ?? (await searchSteamAppId(game, Math.min(deadline, Date.now() + 4_000)));
    if (!resolvedSteamAppId) return game;

    const details = await fetchSteamDetails(resolvedSteamAppId, Math.min(deadline, Date.now() + 3_500));
    if (!details) return {
      ...game,
      externalIds: { ...game.externalIds, steamAppId: resolvedSteamAppId },
      links: {
        ...game.links,
        steamStore: `https://store.steampowered.com/app/${resolvedSteamAppId}`,
        steamWorkshop: `https://steamcommunity.com/app/${resolvedSteamAppId}/workshop/`,
      },
      dataSources: unique([...game.dataSources, "Steam Store search match"]),
      metadataConfidence: Math.max(game.metadataConfidence, 76),
      technicalDetails: {
        ...game.technicalDetails,
        metadataStatus: "Steam app matched; detailed provider request timed out or returned no public data",
      },
    };

    enrichedCount += 1;
    return applySteamDetails(game, resolvedSteamAppId, details);
  });

  progress.push({ phase: "Online metadata enriched", checked, found: enrichedCount });
  return enriched;
}

async function loadSteamAppList(deadline: number): Promise<SteamAppListItem[]> {
  if (steamAppListCache) return steamAppListCache;
  const cachePath = path.join(metadataCacheDir(), "steam-app-list.json");
  const cached = await readJsonCache<{ cachedAt: string; apps: SteamAppListItem[] }>(cachePath);
  if (cached && Date.now() - Date.parse(cached.cachedAt) < 7 * 24 * 60 * 60 * 1000) {
    steamAppListCache = cached.apps;
    return cached.apps;
  }

  if (Date.now() > deadline) return cached?.apps ?? [];

  try {
    const response = await fetchWithTimeout("https://api.steampowered.com/ISteamApps/GetAppList/v2/", deadline);
    const parsed = (await response.json()) as { applist?: { apps?: SteamAppListItem[] } };
    const apps = (parsed.applist?.apps ?? [])
      .filter((app) => app.appid && app.name?.trim())
      .map((app) => ({ appid: Number(app.appid), name: app.name.trim() }));
    steamAppListCache = apps;
    await writeJsonCache(cachePath, { cachedAt: new Date().toISOString(), apps });
    return apps;
  } catch {
    steamAppListCache = cached?.apps ?? [];
    return steamAppListCache;
  }
}

async function fetchSteamDetails(appId: string, deadline: number): Promise<SteamDetails | undefined> {
  const cachePath = path.join(metadataCacheDir(), "steam-details", `${appId}.json`);
  const cached = await readJsonCache<{ cachedAt: string; data?: SteamDetails }>(cachePath);
  if (cached && Date.now() - Date.parse(cached.cachedAt) < 14 * 24 * 60 * 60 * 1000) return cached.data;
  if (Date.now() > deadline) return cached?.data;

  try {
    const url = `https://store.steampowered.com/api/appdetails?appids=${encodeURIComponent(appId)}&filters=basic,genres,categories,screenshots,release_date,developers,publishers`;
    const response = await fetchWithTimeout(url, deadline);
    const parsed = (await response.json()) as Record<string, { success?: boolean; data?: SteamDetails }>;
    const data = parsed[appId]?.success ? parsed[appId]?.data : undefined;
    await writeJsonCache(cachePath, { cachedAt: new Date().toISOString(), data });
    return data;
  } catch {
    return cached?.data;
  }
}

async function searchSteamAppId(game: GameRecord, deadline: number): Promise<string | undefined> {
  const cacheKey = normalizeTitleKey(`${game.title}-${path.basename(game.executablePath, ".exe")}`) || game.id;
  const cachePath = path.join(metadataCacheDir(), "steam-search", `${cacheKey}.json`);
  const cached = await readJsonCache<{ cachedAt: string; appId?: string }>(cachePath);
  if (cached && Date.now() - Date.parse(cached.cachedAt) < 30 * 24 * 60 * 60 * 1000) return cached.appId;

  const terms = unique([
    game.title,
    cleanTitle(path.basename(game.executablePath, ".exe")),
    cleanTitle(path.basename(game.installPath)),
  ]).filter((term) => normalizeTitleKey(term).length >= 4);

  for (const term of terms) {
    if (Date.now() > deadline) break;
    try {
      const url = `https://store.steampowered.com/api/storesearch/?term=${encodeURIComponent(term)}&l=en&cc=US`;
      const response = await fetchWithTimeout(url, deadline);
      const parsed = (await response.json()) as { items?: SteamSearchItem[] };
      const match = pickSteamSearchMatch(game, parsed.items ?? []);
      if (match) {
        await writeJsonCache(cachePath, { cachedAt: new Date().toISOString(), appId: match });
        return match;
      }
    } catch {
      continue;
    }
  }

  await writeJsonCache(cachePath, { cachedAt: new Date().toISOString(), appId: undefined });
  return undefined;
}

function pickSteamSearchMatch(game: GameRecord, items: SteamSearchItem[]): string | undefined {
  const keys = unique([
    normalizeTitleKey(game.title),
    normalizeTitleKey(path.basename(game.executablePath, ".exe")),
    normalizeTitleKey(path.basename(game.installPath)),
  ]).filter((key) => key.length >= 4);

  const scored = items
    .filter((item) => item.type === "app" && item.id && item.name)
    .map((item) => {
      const itemKey = normalizeTitleKey(item.name ?? "");
      const exact = keys.some((key) => itemKey === key);
      const contained = keys.some((key) => key.length >= 7 && (itemKey.includes(key) || key.includes(itemKey)));
      return {
        appId: String(item.id),
        score: exact ? 100 : contained ? 82 : 0,
      };
    })
    .filter((item) => item.score >= 82)
    .sort((a, b) => b.score - a.score);

  return scored[0]?.appId;
}

function applySteamDetails(game: GameRecord, appId: string, details: SteamDetails): GameRecord {
  const categories = details.categories?.map((item) => item.description).filter(Boolean) as string[] | undefined;
  const genres = details.genres?.map((item) => item.description).filter(Boolean) as string[] | undefined;
  const screenshots = details.screenshots?.map((item) => item.path_full ?? item.path_thumbnail).filter(Boolean) as string[] | undefined;
  const hasWorkshop = categories?.some((category) => /workshop/i.test(category)) ?? false;
  const hasController = categories?.some((category) => /controller/i.test(category)) ?? false;
  const steamCdn = {
    hero: `https://cdn.akamai.steamstatic.com/steam/apps/${appId}/library_hero.jpg`,
    cover: `https://cdn.akamai.steamstatic.com/steam/apps/${appId}/library_600x900.jpg`,
  };

  return {
    ...game,
    title: details.name?.trim() || game.title,
    externalIds: { ...game.externalIds, steamAppId: appId },
    links: {
      ...game.links,
      steamStore: `https://store.steampowered.com/app/${appId}`,
      steamWorkshop: hasWorkshop ? `https://steamcommunity.com/app/${appId}/workshop/` : game.links.steamWorkshop,
    },
    dataSources: unique([...game.dataSources, "Steam app list name match", "Steam Store metadata", "Steam CDN artwork"]),
    metadataConfidence: Math.max(game.metadataConfidence, game.externalIds?.steamAppId === appId ? 94 : 86),
    heroImage: details.header_image || steamCdn.hero,
    coverImage: details.capsule_imagev5 || details.capsule_image || steamCdn.cover,
    genres: genres?.length ? genres : game.genres,
    tags: unique([
      ...game.tags,
      ...(categories?.slice(0, 5) ?? []),
      "Steam Store metadata",
      "Provider images",
    ]),
    developers: details.developers ?? game.developers,
    publishers: details.publishers ?? game.publishers,
    releaseDate: details.release_date?.date || game.releaseDate,
    screenshots: screenshots?.slice(0, 6) ?? game.screenshots,
    controllerSupport: game.controllerSupport || hasController,
    cloudSave: game.cloudSave || (categories?.some((category) => /cloud/i.test(category)) ?? false),
    modSupport: hasWorkshop ? "native" : game.modSupport === "unknown" ? "community" : game.modSupport,
    description: details.short_description?.trim() || game.description,
    technicalDetails: {
      ...game.technicalDetails,
      metadataStatus: `Steam metadata matched as app ${appId}; artwork, genres, release data, and mod links attached`,
    },
  };
}

function matchSteamAppId(game: GameRecord, appList: SteamAppListItem[]): string | undefined {
  if (!appList.length) return undefined;
  const keys = unique([
    normalizeTitleKey(game.title),
    normalizeTitleKey(path.basename(game.installPath)),
    normalizeTitleKey(path.basename(game.executablePath, ".exe")),
  ]).filter((key) => key.length >= 4);
  if (!keys.length) return undefined;

  for (const key of keys) {
    const exact = appList.find((app) => normalizeTitleKey(app.name) === key);
    if (exact) return String(exact.appid);
  }

  const strongKey = keys.sort((a, b) => b.length - a.length)[0];
  if (strongKey.length < 8) return undefined;
  const contained = appList.find((app) => {
    const appKey = normalizeTitleKey(app.name);
    return appKey.length >= 8 && (appKey.includes(strongKey) || strongKey.includes(appKey));
  });
  return contained ? String(contained.appid) : undefined;
}

async function findSteamAppId(installPath: string, executablePath: string): Promise<string | undefined> {
  const candidates = unique([
    path.join(installPath, "steam_appid.txt"),
    path.join(path.dirname(executablePath), "steam_appid.txt"),
    path.join(path.dirname(path.dirname(executablePath)), "steam_appid.txt"),
  ]);
  for (const candidate of candidates) {
    try {
      const value = (await fs.readFile(candidate, "utf8")).trim();
      if (/^\d+$/.test(value)) return value;
    } catch {
      continue;
    }
  }
  return undefined;
}

function addGame(discovered: Map<string, GameRecord>, game: GameRecord) {
  const existingEntry = [...discovered.entries()].find(([, existing]) => isSameGame(existing, game));
  if (existingEntry) {
    const [existingKey, existing] = existingEntry;
    if (existing.executableConfidence < game.executableConfidence) {
      discovered.delete(existingKey);
      discovered.set(game.executablePath.toLowerCase(), game);
    }
    return;
  }

  discovered.set(game.executablePath.toLowerCase(), game);
}

function isSameGame(left: GameRecord, right: GameRecord): boolean {
  const leftInstall = normalizePath(left.installPath);
  const rightInstall = normalizePath(right.installPath);
  if (leftInstall === rightInstall) return true;
  if (left.title === right.title && (leftInstall.startsWith(`${rightInstall}\\`) || rightInstall.startsWith(`${leftInstall}\\`))) return true;
  return false;
}

function extractQuotedPaths(content: string): string[] {
  return [...content.matchAll(/"path"\s+"([^"]+)"/gi)].map((match) => match[1]);
}

function extractVdfValue(content: string, key: string): string | undefined {
  return new RegExp(`"${key}"\\s+"([^"]+)"`, "i").exec(content)?.[1];
}

function titleFromPath(input: string): string {
  return cleanTitle(path.basename(input).replace(/\.[^.]+$/, ""));
}

function bestTitleFromFolderAndExe(folderName: string, exe: string): string {
  const folderTitle = titleFromPath(folderName);
  const exeTitle = titleFromPath(path.basename(exe, ".exe"));
  const exeKey = normalizeTitleKey(exeTitle);
  const folderKey = normalizeTitleKey(folderTitle);
  if (exeKey.length > folderKey.length + 2 && !["game", "client", "shipping", "win64"].some((word) => exeKey.includes(word))) {
    return exeTitle;
  }
  return folderTitle;
}

function cleanTitle(title: string): string {
  return title
    .replace(/\bcod\b/gi, "Call of Duty")
    .replace(/([a-z])([A-Z])/g, "$1 $2")
    .replace(/([a-z])(\d)/g, "$1 $2")
    .replace(/(\d)([A-Za-z])/g, "$1 $2")
    .replace(/\b(of|and|the|with|for|in|on)([A-Z])/g, "$1 $2")
    .replace(/[_-]+/g, " ")
    .replace(/\s+/g, " ")
    .replace(/\b\w/g, (char) => char.toUpperCase())
    .trim();
}

function inferGameRoot(exe: string): string {
  const parts = exe.split(path.sep);
  const index = parts.findIndex((part) => GAME_HINTS.includes(part.toLowerCase()));
  if (index >= 0 && parts[index + 1]) return parts.slice(0, index + 2).join(path.sep);
  return path.dirname(exe);
}

function inferGenres(title: string): string[] {
  const lower = title.toLowerCase();
  if (lower.includes("racing") || lower.includes("speed")) return ["Racing", "Arcade"];
  if (lower.includes("war") || lower.includes("strike")) return ["Action", "Shooter"];
  if (lower.includes("quest") || lower.includes("ring")) return ["Adventure", "RPG"];
  return ["PC Game", "Library"];
}

function sourceLabel(source: GameSource): string {
  return source.split("-").map(cleanTitle).join(" ");
}

function isNonGameExe(exe: string): boolean {
  const lower = exe.toLowerCase();
  const base = path.basename(exe).toLowerCase();
  if (lower.includes("\\_redist\\") || lower.includes("\\redist\\") || lower.includes("\\redistributable")) return true;
  if (lower.includes("\\support\\") || lower.includes("\\installer\\")) return true;
  return NON_GAME_EXES.some((blocked) => base.includes(blocked));
}

function isLikelyGamePath(candidate: string): boolean {
  const lower = candidate.toLowerCase();
  return (
    GAME_HINTS.some((hint) => lower.includes(hint)) &&
    !lower.includes("gamesave manager") &&
    !lower.includes("windows\\system32") &&
    !lower.includes("administrative tools")
  );
}

function normalizeRegistryPath(value: string | undefined): string | undefined {
  const trimmed = value?.trim().replace(/^"|"$/g, "");
  return trimmed && /^[A-Za-z]:\\/.test(trimmed) ? trimmed : undefined;
}

function normalizeDisplayIcon(value: string | undefined): string | undefined {
  const trimmed = value?.trim();
  if (!trimmed) return undefined;
  const match = /^"?([A-Za-z]:\\.+?\.exe)"?(?:,\d+)?$/i.exec(trimmed);
  return match?.[1];
}

function isLikelyRegistryGame(row: Record<string, string | undefined>, installPath?: string, displayIcon?: string): boolean {
  const haystack = [
    row.DisplayName,
    row.Publisher,
    row.UninstallString,
    installPath,
    displayIcon,
  ].filter(Boolean).join(" ").toLowerCase();
  if (!haystack || haystack.includes("microsoft visual c++") || haystack.includes(".net runtime")) return false;
  return GAME_HINTS.some((hint) => haystack.includes(hint)) || GAME_PUBLISHER_HINTS.some((hint) => haystack.includes(hint));
}

function shouldSkipDirectory(name: string): boolean {
  return [
    ".git",
    "node_modules",
    "$recycle.bin",
    "windows",
    "programdata",
    "appdata",
    "system volume information",
    "_commonredist",
    "_redist",
    "redist",
    "redistributables",
    "support",
    "tools",
  ].includes(name.toLowerCase());
}

function shouldSkipGameFolder(name: string): boolean {
  return ["_commonredist", "_redist", "redist", "redistributables", "trainers"].includes(name.toLowerCase());
}

function isContainerGameFolder(name: string): boolean {
  return ["nowemod", "no wemod", "roms", "emulators"].includes(name.toLowerCase());
}

async function exists(filePath: string): Promise<boolean> {
  try {
    await fs.access(filePath);
    return true;
  } catch {
    return false;
  }
}

async function safeReaddir(dir: string): Promise<string[]> {
  try {
    return await fs.readdir(dir);
  } catch {
    return [];
  }
}

async function safeDirents(dir: string) {
  try {
    return await fs.readdir(dir, { withFileTypes: true });
  } catch {
    return [];
  }
}

async function safeStat(filePath: string) {
  try {
    return await fs.stat(filePath);
  } catch {
    return undefined;
  }
}

function unique(values: string[]): string[] {
  return [...new Set(values)];
}

function normalizePath(value: string): string {
  return value.replace(/[\\/]+$/, "").toLowerCase();
}

function hash(value: string): string {
  let h = 2166136261;
  for (let i = 0; i < value.length; i += 1) {
    h ^= value.charCodeAt(i);
    h = Math.imul(h, 16777619);
  }
  return `game-${(h >>> 0).toString(16)}`;
}

function normalizeTitleKey(value: string): string {
  return value.toLowerCase().replace(/[^a-z0-9]/g, "");
}

function metadataCacheDir(): string {
  return path.join(process.env.LOCALAPPDATA ?? os.tmpdir(), "VaultPlay", "metadata-cache");
}

async function readJsonCache<T>(filePath: string): Promise<T | undefined> {
  try {
    return JSON.parse(await fs.readFile(filePath, "utf8")) as T;
  } catch {
    return undefined;
  }
}

async function writeJsonCache(filePath: string, value: unknown): Promise<void> {
  try {
    await fs.mkdir(path.dirname(filePath), { recursive: true });
    await fs.writeFile(filePath, JSON.stringify(value), "utf8");
  } catch {
    // Cache writes are best effort; discovery must not fail because metadata caching failed.
  }
}

async function fetchWithTimeout(url: string, deadline: number): Promise<Response> {
  const timeoutMs = Math.max(1, Math.min(8_000, deadline - Date.now()));
  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), timeoutMs);
  try {
    const response = await fetch(url, {
      signal: controller.signal,
      headers: {
        "Accept": "application/json",
        "User-Agent": "VaultPlay-GameLauncher/0.1",
      },
    });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response;
  } finally {
    clearTimeout(timeout);
  }
}

async function mapLimit<T, R>(items: T[], limit: number, worker: (item: T) => Promise<R>): Promise<R[]> {
  const results = new Array<R>(items.length);
  let index = 0;

  async function run() {
    while (index < items.length) {
      const current = index;
      index += 1;
      results[current] = await worker(items[current]);
    }
  }

  await Promise.all(Array.from({ length: Math.min(limit, items.length) }, run));
  return results;
}

function gradient(seed: string, hero: boolean): string {
  const palettes = [
    ["#08111f", "#36d5ff", "#ffb15e"],
    ["#130c1c", "#ff5578", "#ffd37a"],
    ["#091b18", "#48f0a7", "#d8b66a"],
    ["#121722", "#80a7ff", "#f05d5e"],
  ];
  const palette = palettes[parseInt(seed.replace(/\D/g, "").slice(0, 2) || "1", 10) % palettes.length];
  const svg = `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${hero ? 1200 : 520} ${hero ? 680 : 740}">
    <defs><radialGradient id="g" cx="60%" cy="20%" r="80%"><stop offset="0" stop-color="${palette[1]}"/><stop offset="0.45" stop-color="${palette[2]}"/><stop offset="1" stop-color="${palette[0]}"/></radialGradient></defs>
    <rect width="100%" height="100%" fill="url(#g)"/>
    <circle cx="74%" cy="28%" r="180" fill="rgba(255,255,255,.18)"/>
    <path d="M0 ${hero ? 520 : 590} C260 410 430 720 720 500 S980 310 1200 440 V900 H0Z" fill="rgba(0,0,0,.34)"/>
  </svg>`;
  return `data:image/svg+xml,${encodeURIComponent(svg)}`;
}
