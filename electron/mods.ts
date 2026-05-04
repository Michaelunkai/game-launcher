import { promises as fs } from "node:fs";
import path from "node:path";
import type {
  GameMod,
  GameModCatalog,
  GameRecord,
  ModActivationMode,
  ModActivationState,
  ModInstallSupport,
  ModIntegrationStatus,
  ModProviderId,
} from "../shared/types.js";

type ModStateFile = Record<string, Record<string, { activationState: ModActivationState; lastChangedAt: string }>>;
const validActivationStates = new Set<ModActivationState>(["enabled", "disabled"]);

export async function readModState(filePath: string): Promise<ModStateFile> {
  try {
    return normalizeModState(JSON.parse(await fs.readFile(filePath, "utf8")));
  } catch {
    return {};
  }
}

export async function writeModState(filePath: string, state: ModStateFile): Promise<void> {
  const dir = path.dirname(filePath);
  const tempPath = path.join(dir, `.${path.basename(filePath)}.${process.pid}.${Date.now()}.tmp`);
  await fs.mkdir(dir, { recursive: true });
  await fs.writeFile(tempPath, JSON.stringify(state, null, 2), "utf8");
  try {
    await replaceFileWithRetry(tempPath, filePath);
  } catch (error) {
    await fs.unlink(tempPath).catch(() => undefined);
    throw error;
  }
}

async function replaceFileWithRetry(tempPath: string, filePath: string): Promise<void> {
  let lastError: unknown;
  for (let attempt = 0; attempt < 8; attempt += 1) {
    try {
      await fs.rename(tempPath, filePath);
      return;
    } catch (error) {
      lastError = error;
      if (!isReplaceRetryable(error)) throw error;
      try {
        await fs.copyFile(tempPath, filePath);
        await fs.unlink(tempPath).catch(() => undefined);
        return;
      } catch (copyError) {
        lastError = copyError;
        if (!isReplaceRetryable(copyError)) throw copyError;
        await delay(40 + attempt * 35);
      }
    }
  }
  throw lastError;
}

function isReplaceRetryable(error: unknown): boolean {
  if (!isRecord(error)) return false;
  return error.code === "EPERM" || error.code === "EACCES" || error.code === "EBUSY" || error.code === "EEXIST";
}

function delay(milliseconds: number): Promise<void> {
  return new Promise((resolve) => setTimeout(resolve, milliseconds));
}

export function buildModCatalog(game: GameRecord, state: ModStateFile): GameModCatalog {
  const gameState = state[game.id] ?? {};
  const sources = buildProviderSources(game).map((source) => {
    const saved = gameState[source.id];
    return {
      ...source,
      activationState: saved?.activationState ?? "disabled",
      lastChangedAt: saved?.lastChangedAt,
    };
  });

  return {
    gameId: game.id,
    gameTitle: game.title,
    mods: sources,
    refreshedAt: new Date().toISOString(),
    summary: {
      totalSources: sources.length,
      enabled: sources.filter((mod) => mod.activationState === "enabled").length,
      disabled: sources.filter((mod) => mod.activationState === "disabled").length,
      providerManaged: sources.filter((mod) => mod.installSupport === "provider-managed").length,
      apiConnectable: sources.filter((mod) => mod.integrationStatus === "api-connectable").length,
      adapterRequired: sources.filter((mod) => mod.installSupport === "adapter-required").length,
      referenceOnly: sources.filter((mod) => mod.integrationStatus === "reference-only").length,
      verifiedProvider: sources.filter((mod) => mod.integrationStatus === "verified-provider").length,
      needsGameAdapter: sources.filter((mod) => mod.integrationStatus === "needs-game-adapter").length,
    },
  };
}

export async function setModActivation(filePath: string, game: GameRecord, modId: string, enabled: boolean): Promise<GameModCatalog> {
  const state = await readModState(filePath);
  const catalog = buildModCatalog(game, state);
  if (!catalog.mods.some((mod) => mod.id === modId)) throw new Error("Mod source not found for this game.");

  state[game.id] = {
    ...(state[game.id] ?? {}),
    [modId]: {
      activationState: enabled ? "enabled" : "disabled",
      lastChangedAt: new Date().toISOString(),
    },
  };
  await writeModState(filePath, state);
  return buildModCatalog(game, state);
}

function buildProviderSources(game: GameRecord): Omit<GameMod, "activationState" | "lastChangedAt">[] {
  const title = game.title;
  const encodedTitle = encodeURIComponent(title);
  const sources: Array<{
    key: string;
    provider: ModProviderId;
    providerName: string;
    title: string;
    summary: string;
    browseUrl: string;
    installSupport: ModInstallSupport;
    integrationStatus: ModIntegrationStatus;
    activationMode: ModActivationMode;
    installStrategy: string;
    safetyNotes: string[];
  }> = [];

  const singlePlayerSafety = [
    "Use only in offline/single-player sessions unless the game explicitly allows mods online.",
    "Exact activation still requires a supported trainer, script table, provider item, or game-specific adapter.",
  ];

  const trainerSearch = (effect: string) =>
    `https://www.google.com/search?q=${encodeURIComponent(`${title} ${effect} WeMod trainer Cheat Engine table`)}`;
  const modSearch = (effect: string) =>
    `https://www.google.com/search?q=${encodeURIComponent(`${title} ${effect} mod Nexus ModDB PCGamingWiki`)}`;

  const gameplayMods: Array<{
    key: string;
    title: string;
    provider: ModProviderId;
    providerName: string;
    summary: string;
    browseUrl: string;
    installSupport: ModInstallSupport;
  }> = [
    {
      key: "unlimited-health",
      title: "Unlimited Health / God Mode",
      provider: "wemod",
      providerName: "Trainer",
      summary: "Looks for supported health-lock, god-mode, or no-damage trainers/tables for this exact game.",
      browseUrl: trainerSearch("Unlimited Health God Mode"),
      installSupport: "adapter-required",
    },
    {
      key: "one-hit-kill",
      title: "One Hit Kill",
      provider: "wemod",
      providerName: "Trainer",
      summary: "Finds trainer or cheat-table entries that make enemies die in one hit when supported.",
      browseUrl: trainerSearch("One Hit Kill"),
      installSupport: "adapter-required",
    },
    {
      key: "infinite-ammo",
      title: "Infinite Ammo / No Reload",
      provider: "cheat-engine",
      providerName: "Cheat Table",
      summary: "Searches for ammo, magazine, reload, durability, or consumable-lock cheats for compatible games.",
      browseUrl: trainerSearch("Infinite Ammo No Reload"),
      installSupport: "adapter-required",
    },
    {
      key: "unlimited-money",
      title: "Unlimited Money / Resources",
      provider: "cheat-engine",
      providerName: "Cheat Table",
      summary: "Finds currency, resource, item, crafting material, score, or inventory editor style mods.",
      browseUrl: trainerSearch("Unlimited Money Resources"),
      installSupport: "adapter-required",
    },
    {
      key: "xp-multiplier",
      title: "XP Multiplier / Fast Leveling",
      provider: "trainer-catalog",
      providerName: "Trainer",
      summary: "Searches for XP, skill point, level, reputation, or progression multipliers.",
      browseUrl: trainerSearch("XP Multiplier Skill Points"),
      installSupport: "adapter-required",
    },
    {
      key: "unlock-all",
      title: "Unlock All / All Collectibles",
      provider: "trainer-catalog",
      providerName: "Trainer",
      summary: "Finds unlock-all, all characters, all levels, collectibles, cosmetics, or save unlock tools.",
      browseUrl: trainerSearch("Unlock All Collectibles Save Editor"),
      installSupport: "adapter-required",
    },
    {
      key: "infinite-stamina",
      title: "Infinite Stamina / No Cooldown",
      provider: "wemod",
      providerName: "Trainer",
      summary: "Looks for stamina, cooldown, ability timer, mana, energy, or special-meter cheats.",
      browseUrl: trainerSearch("Infinite Stamina No Cooldown"),
      installSupport: "adapter-required",
    },
    {
      key: "speed-time-scale",
      title: "Game Speed / Time Scale",
      provider: "trainer-catalog",
      providerName: "Trainer",
      summary: "Finds game-speed, slow-motion, time-scale, walk-speed, sprint-speed, or super-speed toggles.",
      browseUrl: trainerSearch("Game Speed Super Speed Time Scale"),
      installSupport: "adapter-required",
    },
    {
      key: "save-editor",
      title: "Save Editor / Inventory Editor",
      provider: "community-fix",
      providerName: "Save Tool",
      summary: "Searches for real save editors, inventory editors, config tools, and backup-safe save utilities.",
      browseUrl: modSearch("save editor inventory editor"),
      installSupport: "adapter-required",
    },
    {
      key: "graphics-hd-textures",
      title: "HD Textures / Graphics Overhaul",
      provider: "nexus-mods",
      providerName: "Nexus / ModDB",
      summary: "Finds HD texture packs, ReShade presets, lighting overhauls, model swaps, and visual upgrades.",
      browseUrl: modSearch("HD texture pack graphics overhaul ReShade"),
      installSupport: "adapter-required",
    },
    {
      key: "fov-ultrawide-fps",
      title: "FOV / Ultrawide / FPS Unlock",
      provider: "pcgamingwiki",
      providerName: "PC Fix",
      summary: "Finds widescreen, ultrawide, FOV, high-FPS, intro skip, and PC compatibility fixes.",
      browseUrl: game.links.pcGamingWiki ?? modSearch("FOV ultrawide FPS unlock fix"),
      installSupport: "adapter-required",
    },
    {
      key: "community-patch",
      title: "Community Patch / Bug Fixes",
      provider: "community-fix",
      providerName: "Community Fix",
      summary: "Searches for unofficial patches, bug fixes, crash fixes, restored content, and QoL patches.",
      browseUrl: modSearch("community patch bug fix quality of life"),
      installSupport: "adapter-required",
    },
  ];

  for (const mod of gameplayMods) {
    sources.push({
      ...mod,
      installStrategy:
        "VaultPlay stores the permanent desired state and opens the best matching trainer/mod search. Automatic in-game activation requires a verified provider item or a game-specific adapter for this exact installed EXE.",
      safetyNotes: singlePlayerSafety,
      integrationStatus: "needs-game-adapter",
      activationMode: "adapter-required",
    });
  }

  if (game.externalIds?.steamAppId) {
    sources.push({
      key: "steam-workshop",
      provider: "steam-workshop",
      providerName: "Steam Workshop",
      title: "Steam Workshop Mods",
      summary: "Official Steam Workshop hub when the game exposes Workshop content.",
      browseUrl: `https://steamcommunity.com/app/${game.externalIds.steamAppId}/workshop/`,
      installSupport: "provider-managed",
      integrationStatus: "verified-provider",
      activationMode: "provider-managed",
      installStrategy: "Steam owns subscription, download, update, and uninstall. VaultPlay stores the permanent desired state and launches the provider-managed hub.",
      safetyNotes: ["Requires Steam account/session for subscription.", "Compatibility still depends on each Workshop item and game version."],
    });
  }

  sources.push(
    {
      key: "nexus-mods",
      provider: "nexus-mods",
      providerName: "Nexus Mods",
      title: "Nexus Mods Search",
      summary: "Searches Nexus for community mods, collections, files, and game-specific pages.",
      browseUrl: game.links.nexusModsSearch ?? `https://www.nexusmods.com/search/?gsearch=${encodedTitle}`,
      installSupport: "adapter-required",
      integrationStatus: "needs-game-adapter",
      activationMode: "adapter-required",
      installStrategy: "Nexus downloads need user authentication plus a game-specific installer/loader adapter before one-click install can be validated.",
      safetyNotes: ["No blind file copying into the game directory.", "Dependencies and load order must be resolved per game."],
    },
    {
      key: "moddb",
      provider: "moddb",
      providerName: "ModDB",
      title: "ModDB Total Conversions",
      summary: "Searches ModDB for total conversions, patches, addons, and standalone mod pages.",
      browseUrl: game.links.modDbSearch ?? `https://www.moddb.com/search?q=${encodedTitle}`,
      installSupport: "adapter-required",
      integrationStatus: "needs-game-adapter",
      activationMode: "adapter-required",
      installStrategy: "ModDB packages vary by game; VaultPlay records enablement and requires a per-game installer adapter for safe activation.",
      safetyNotes: ["Manual archives can contain arbitrary layouts.", "Game backups and compatibility checks are required before applying files."],
    },
    {
      key: "modio",
      provider: "modio",
      providerName: "mod.io",
      title: "mod.io Official Mods",
      summary: "Finds games that expose official mod.io ecosystems and subscription-based mod flows.",
      browseUrl: `https://mod.io/games?filter=t&kw=${encodedTitle}`,
      installSupport: "api-ready",
      integrationStatus: "api-connectable",
      activationMode: "saved-preference",
      installStrategy: "mod.io supports API/SDK subscription flows, but write actions require OAuth and game IDs; VaultPlay persists the desired state until connected.",
      safetyNotes: ["Subscribe/unsubscribe requires authenticated provider access.", "Some games sync mods only through their own in-game mod menu."],
    },
    {
      key: "curseforge",
      provider: "curseforge",
      providerName: "CurseForge",
      title: "CurseForge Addons",
      summary: "Searches CurseForge for supported games, addons, mods, maps, and resource packs.",
      browseUrl: `https://www.curseforge.com/search?search=${encodedTitle}`,
      installSupport: "api-ready",
      integrationStatus: "api-connectable",
      activationMode: "saved-preference",
      installStrategy: "CurseForge API access requires an API key and per-game file placement rules before automated enablement.",
      safetyNotes: ["API key must stay outside source code.", "Provider categories and game versions must match the installed game."],
    },
    {
      key: "thunderstore",
      provider: "thunderstore",
      providerName: "Thunderstore",
      title: "Thunderstore Modpacks",
      summary: "Searches Thunderstore for BepInEx/R2ModMan-style communities and dependency-based modpacks.",
      browseUrl: `https://thunderstore.io/?q=${encodedTitle}`,
      installSupport: "adapter-required",
      integrationStatus: "needs-game-adapter",
      activationMode: "adapter-required",
      installStrategy: "Thunderstore mods often require BepInEx and dependency resolution; VaultPlay needs a game adapter before one-click activation.",
      safetyNotes: ["Dependencies must be installed in order.", "Loader version must match the game build."],
    },
    {
      key: "pcgamingwiki",
      provider: "pcgamingwiki",
      providerName: "PCGamingWiki",
      title: "PCGamingWiki Fixes",
      summary: "Finds PC-specific fixes, save locations, modding notes, and compatibility warnings.",
      browseUrl: game.links.pcGamingWiki ?? `https://www.pcgamingwiki.com/w/index.php?search=${encodedTitle}`,
      installSupport: "adapter-required",
      integrationStatus: "reference-only",
      activationMode: "saved-preference",
      installStrategy: "PCGamingWiki is used as compatibility intelligence before enabling invasive mods.",
      safetyNotes: ["Useful for requirements, save locations, launch arguments, and known conflicts.", "Not a mod download host by itself."],
    },
  );

  return sources.map((source) => ({
    id: `${game.id}:${source.key}`,
    gameId: game.id,
    ...source,
  }));
}

function normalizeModState(value: unknown): ModStateFile {
  if (!isRecord(value)) return {};

  const normalized: ModStateFile = {};
  for (const [gameId, gameValue] of Object.entries(value)) {
    if (!isRecord(gameValue)) continue;

    const gameState: ModStateFile[string] = {};
    for (const [modId, modValue] of Object.entries(gameValue)) {
      if (!isRecord(modValue)) continue;
      if (!validActivationStates.has(modValue.activationState as ModActivationState)) continue;
      if (typeof modValue.lastChangedAt !== "string") continue;

      gameState[modId] = {
        activationState: modValue.activationState as ModActivationState,
        lastChangedAt: modValue.lastChangedAt,
      };
    }

    if (Object.keys(gameState).length > 0) normalized[gameId] = gameState;
  }

  return normalized;
}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === "object" && value !== null && !Array.isArray(value);
}
