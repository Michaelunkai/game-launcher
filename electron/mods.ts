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

  if (game.externalIds?.steamAppId) {
    sources.push({
      key: "steam-workshop",
      provider: "steam-workshop",
      providerName: "Steam Workshop",
      title: "Workshop maps, levels, and subscribed content",
      summary: "Adds the game's official Workshop items such as maps, levels, missions, campaigns, and community content.",
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
      title: "Gameplay, texture, UI, and quality-of-life mods",
      summary: "Adds Nexus-hosted gameplay changes, HD textures, UI improvements, reshades, fixes, and community patches.",
      browseUrl: game.links.nexusModsSearch ?? `https://www.nexusmods.com/search/?gsearch=${encodedTitle}`,
      installSupport: "provider-managed",
      integrationStatus: "verified-provider",
      activationMode: "provider-managed",
      installStrategy: "Nexus owns file pages, requirements, dependencies, and Vortex/manual install instructions. VaultPlay saves the preference and opens the real provider route.",
      safetyNotes: ["Use the provider page requirements before installing.", "VaultPlay does not blindly copy files into game folders."],
    },
    {
      key: "moddb",
      provider: "moddb",
      providerName: "ModDB",
      title: "Total conversions, expansions, and patch packs",
      summary: "Adds ModDB-hosted total conversions, expansion-style projects, addon packs, overhaul mods, and patch releases.",
      browseUrl: game.links.modDbSearch ?? `https://www.moddb.com/search?q=${encodedTitle}`,
      installSupport: "provider-managed",
      integrationStatus: "verified-provider",
      activationMode: "provider-managed",
      installStrategy: "ModDB owns download pages, install notes, and version details. VaultPlay saves the preference and opens the real provider route.",
      safetyNotes: ["Read the provider install notes before applying archives.", "Back up saves before using total conversions or patches."],
    },
    {
      key: "modio",
      provider: "modio",
      providerName: "mod.io",
      title: "Official subscribed mods and in-game content",
      summary: "Adds mod.io ecosystem content such as subscribed mods, maps, cosmetics, scenarios, and game-supported community items.",
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
      title: "Addons, resource packs, maps, and mod files",
      summary: "Adds CurseForge-supported addons, resource packs, maps, mod files, dependencies, and versioned game content.",
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
      title: "BepInEx modpacks, plugins, and dependencies",
      summary: "Adds Thunderstore/R2ModMan-style plugin mods, BepInEx packs, dependency bundles, and multiplayer-safe profiles where supported.",
      browseUrl: `https://thunderstore.io/?q=${encodedTitle}`,
      installSupport: "provider-managed",
      integrationStatus: "verified-provider",
      activationMode: "provider-managed",
      installStrategy: "Thunderstore owns package pages, dependencies, and mod manager routing. VaultPlay saves the preference and opens the real provider route.",
      safetyNotes: ["Use Thunderstore/R2ModMan dependency handling where available.", "Loader version must match the game build."],
    },
    {
      key: "pcgamingwiki",
      provider: "pcgamingwiki",
      providerName: "PCGamingWiki",
      title: "PC fixes, widescreen tweaks, FPS fixes, and config notes",
      summary: "Shows practical PC fixes such as ultrawide/FOV guidance, FPS fixes, save/config locations, launch arguments, and compatibility notes.",
      browseUrl: game.links.pcGamingWiki ?? `https://www.pcgamingwiki.com/w/index.php?search=${encodedTitle}`,
      installSupport: "provider-managed",
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
