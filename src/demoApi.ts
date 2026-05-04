import type { GameModCatalog, GameRecord, LauncherApi, ScanResult } from "../shared/types";

const now = new Date().toISOString();

const demoGames = [
  {
    id: "demo-1",
    title: "Astral Siege",
    executablePath: "F:\\Games\\Astral Siege\\AstralSiege-Win64-Shipping.exe",
    installPath: "F:\\Games\\Astral Siege",
    drive: "F:\\",
    source: "demo",
    dataSources: ["Demo fallback"],
    matchStatus: "verified",
    executableConfidence: 98,
    metadataConfidence: 94,
    totalPlaySeconds: 212400,
    lastPlayedAt: now,
    firstDiscoveredAt: now,
    heroImage:
      "data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 1200 680'%3E%3Cdefs%3E%3CradialGradient id='g' cx='70%25' cy='20%25' r='80%25'%3E%3Cstop stop-color='%2380d8ff'/%3E%3Cstop offset='.48' stop-color='%23ff9b55'/%3E%3Cstop offset='1' stop-color='%23060b17'/%3E%3C/radialGradient%3E%3C/defs%3E%3Crect width='1200' height='680' fill='url(%23g)'/%3E%3Cpath d='M0 510 C260 360 460 720 760 480 S1010 330 1200 420 V680 H0Z' fill='rgba(0,0,0,.38)'/%3E%3C/svg%3E",
    coverImage:
      "data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 520 740'%3E%3Cdefs%3E%3ClinearGradient id='g' x1='0' y1='0' x2='1' y2='1'%3E%3Cstop stop-color='%23081727'/%3E%3Cstop offset='.5' stop-color='%233ad6ff'/%3E%3Cstop offset='1' stop-color='%23ff9f59'/%3E%3C/linearGradient%3E%3C/defs%3E%3Crect width='520' height='740' fill='url(%23g)'/%3E%3Ccircle cx='360' cy='160' r='120' fill='rgba(255,255,255,.22)'/%3E%3C/svg%3E",
    genres: ["Action", "Sci-Fi"],
    tags: ["Auto-discovered", "Correct EXE", "IGDB-ready"],
    platforms: ["Windows"],
    controllerSupport: true,
    cloudSave: true,
    modSupport: "community",
    description: "A verified local install with metadata and images ready to sync from configured providers.",
    technicalDetails: {
      executableName: "AstralSiege-Win64-Shipping.exe",
      executableSizeMb: 128.4,
      lastModifiedAt: now,
      metadataStatus: "Demo fallback only",
    },
  },
  {
    id: "demo-2",
    title: "Velvet Rally",
    executablePath: "D:\\Launchers\\Racing\\VelvetRally.exe",
    installPath: "D:\\Launchers\\Racing",
    drive: "D:\\",
    source: "demo",
    dataSources: ["Demo fallback"],
    matchStatus: "likely",
    executableConfidence: 87,
    metadataConfidence: 81,
    totalPlaySeconds: 54300,
    firstDiscoveredAt: now,
    heroImage:
      "data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 1200 680'%3E%3Crect width='1200' height='680' fill='%23120f18'/%3E%3Cellipse cx='720' cy='240' rx='420' ry='190' fill='%23ffcf6b' opacity='.72'/%3E%3Cellipse cx='390' cy='460' rx='390' ry='150' fill='%23ff4f71' opacity='.45'/%3E%3C/svg%3E",
    coverImage:
      "data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 520 740'%3E%3Crect width='520' height='740' fill='%23120f18'/%3E%3Cpath d='M0 450 C130 330 290 650 520 390 V740 H0Z' fill='%23ffcf6b'/%3E%3C/svg%3E",
    genres: ["Racing", "Arcade"],
    tags: ["Drive D:", "Playable", "Review match"],
    platforms: ["Windows"],
    controllerSupport: true,
    cloudSave: false,
    modSupport: "unknown",
    description: "A high-confidence executable found from a game-like path, ready for manual confirmation.",
    technicalDetails: {
      executableName: "VelvetRally.exe",
      executableSizeMb: 92.1,
      lastModifiedAt: now,
      metadataStatus: "Demo fallback only",
    },
  },
  {
    id: "demo-3",
    title: "Cobalt Kingdoms",
    executablePath: "C:\\XboxGames\\Cobalt Kingdoms\\Content\\Game.exe",
    installPath: "C:\\XboxGames\\Cobalt Kingdoms",
    drive: "C:\\",
    source: "demo",
    dataSources: ["Demo fallback"],
    matchStatus: "verified",
    executableConfidence: 91,
    metadataConfidence: 89,
    totalPlaySeconds: 91800,
    firstDiscoveredAt: now,
    heroImage:
      "data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 1200 680'%3E%3Crect width='1200' height='680' fill='%23070f18'/%3E%3Ccircle cx='710' cy='270' r='270' fill='%2380a7ff' opacity='.78'/%3E%3Cpath d='M0 580 C300 410 520 610 820 450 S1050 390 1200 470 V680 H0Z' fill='%23090f13'/%3E%3C/svg%3E",
    coverImage:
      "data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 520 740'%3E%3Crect width='520' height='740' fill='%23070f18'/%3E%3Ccircle cx='300' cy='230' r='180' fill='%2380a7ff'/%3E%3C/svg%3E",
    genres: ["Strategy", "Adventure"],
    tags: ["XboxGames", "Metadata matched", "Cloud save"],
    platforms: ["Windows"],
    controllerSupport: false,
    cloudSave: true,
    modSupport: "native",
    description: "A store-style install detected with strong metadata confidence and playtime tracking enabled.",
    technicalDetails: {
      executableName: "Game.exe",
      executableSizeMb: 104.6,
      lastModifiedAt: now,
      metadataStatus: "Demo fallback only",
    },
  },
];

const previewApiBase = "http://127.0.0.1:5274/api";

async function request<T>(path: string, init?: RequestInit): Promise<T> {
  const response = await fetch(`${previewApiBase}${path}`, {
    ...init,
    headers: {
      "Content-Type": "application/json",
      ...init?.headers,
    },
  });
  if (!response.ok) throw new Error(`Preview API ${response.status}`);
  return (await response.json()) as T;
}

export const demoApi: LauncherApi = {
  getLibrary: async () => {
    try {
      return await request<GameRecord[]>("/library");
    } catch {
      return [];
    }
  },
  scanGames: async (): Promise<ScanResult> => {
    try {
      return await request<ScanResult>("/scan", { method: "POST" });
    } catch {
      return {
        games: [],
        progress: [
          { phase: "REAL API OFFLINE: start npm run preview:api to scan installed games", checked: 0, found: 0 },
        ],
        scannedDrives: [],
        completedAt: new Date().toISOString(),
      };
    }
  },
  launchGame: async (gameId) => {
    try {
      return await request<{ ok: boolean; message: string }>(`/launch/${encodeURIComponent(gameId)}`, { method: "POST" });
    } catch {
      return { ok: false, message: "Real preview API is offline, so browser preview cannot launch this EXE yet." };
    }
  },
  getMods: async (gameId) => {
    try {
      return await request<GameModCatalog>(`/mods/${encodeURIComponent(gameId)}`);
    } catch {
      return {
        gameId,
        gameTitle: "Unknown game",
        refreshedAt: new Date().toISOString(),
        mods: [],
        summary: {
          totalSources: 0,
          enabled: 0,
          disabled: 0,
          providerManaged: 0,
          apiConnectable: 0,
          adapterRequired: 0,
          referenceOnly: 0,
          verifiedProvider: 0,
          needsGameAdapter: 0,
        },
      };
    }
  },
  setModEnabled: async (gameId, modId, enabled) => {
    try {
      return await request<{ ok: boolean; message: string; catalog?: GameModCatalog }>(
        `/mods/${encodeURIComponent(gameId)}/${encodeURIComponent(modId)}/${enabled ? "enable" : "disable"}`,
        { method: "POST" },
      );
    } catch {
      return { ok: false, message: "Real preview API is offline, so mod state cannot be changed." };
    }
  },
};
