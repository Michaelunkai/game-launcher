import { app, BrowserWindow, ipcMain } from "electron";
import { promises as fs } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import type { GameRecord } from "../shared/types.js";
import { launchExecutable } from "./launcher.js";
import { buildModCatalog, readModState, setModActivation } from "./mods.js";
import { scanInstalledGames } from "./scanner.js";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const isDev = !app.isPackaged;
let mainWindow: BrowserWindow | undefined;
let libraryCache: GameRecord[] = [];
const activeSessions = new Map<string, number>();

function dataFile() {
  return path.join(app.getPath("userData"), "library.json");
}

function modStateFile() {
  return path.join(app.getPath("userData"), "mods.json");
}

async function readLibrary(): Promise<GameRecord[]> {
  try {
    const parsed = JSON.parse(await fs.readFile(dataFile(), "utf8")) as GameRecord[];
    libraryCache = parsed;
    return parsed;
  } catch {
    libraryCache = [];
    return [];
  }
}

async function writeLibrary(games: GameRecord[]) {
  libraryCache = games;
  await fs.mkdir(path.dirname(dataFile()), { recursive: true });
  await fs.writeFile(dataFile(), JSON.stringify(games, null, 2), "utf8");
}

async function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1500,
    height: 930,
    minWidth: 1120,
    minHeight: 720,
    backgroundColor: "#05070d",
    title: "Game Launcher",
    webPreferences: {
      preload: path.join(__dirname, "preload.js"),
      nodeIntegration: false,
      contextIsolation: true,
      sandbox: false,
    },
  });

  if (isDev) {
    await mainWindow.loadURL("http://127.0.0.1:5173");
  } else {
    await mainWindow.loadFile(path.join(__dirname, "..", "dist", "index.html"));
  }
}

app.whenReady().then(async () => {
  await readLibrary();
  await createWindow();

  app.on("activate", () => {
    if (BrowserWindow.getAllWindows().length === 0) void createWindow();
  });
});

app.on("window-all-closed", () => {
  if (process.platform !== "darwin") app.quit();
});

ipcMain.handle("library:get", async () => readLibrary());

ipcMain.handle("library:scan", async () => {
  const result = await scanInstalledGames();
  const existing = new Map(libraryCache.map((game) => [game.executablePath.toLowerCase(), game]));
  const merged = result.games.map((game) => ({
    ...game,
    totalPlaySeconds: existing.get(game.executablePath.toLowerCase())?.totalPlaySeconds ?? game.totalPlaySeconds,
    lastPlayedAt: existing.get(game.executablePath.toLowerCase())?.lastPlayedAt,
  }));
  await writeLibrary(merged);
  return { ...result, games: merged };
});

ipcMain.handle("mods:get", async (_event, gameId: string) => {
  const game = libraryCache.find((item) => item.id === gameId);
  if (!game) throw new Error("Game not found in local library.");
  return buildModCatalog(game, await readModState(modStateFile()));
});

ipcMain.handle("mods:set-enabled", async (_event, gameId: string, modId: string, enabled: boolean) => {
  const game = libraryCache.find((item) => item.id === gameId);
  if (!game) return { ok: false, message: "Game not found in local library." };
  try {
    const catalog = await setModActivation(modStateFile(), game, modId, enabled);
    const mod = catalog.mods.find((item) => item.id === modId);
    return {
      ok: true,
      message: `${enabled ? "Enabled" : "Disabled"} ${mod?.title ?? "mod"} permanently for ${game.title}.`,
      catalog,
    };
  } catch (error) {
    return { ok: false, message: error instanceof Error ? error.message : "Could not update mod state." };
  }
});

ipcMain.handle("game:launch", async (_event, gameId: string) => {
  const game = libraryCache.find((item) => item.id === gameId);
  if (!game) return { ok: false, message: "Game not found in local library." };

  try {
    const modCatalog = buildModCatalog(game, await readModState(modStateFile()));
    const startedAt = Date.now();
    const child = await launchExecutable(game);
    activeSessions.set(game.id, startedAt);

    child.once("error", () => activeSessions.delete(game.id));
    child.once("exit", async () => {
      const start = activeSessions.get(game.id);
      if (!start) return;
      activeSessions.delete(game.id);
      const elapsed = Math.max(0, Math.round((Date.now() - start) / 1000));
      libraryCache = libraryCache.map((item) =>
        item.id === game.id
          ? { ...item, totalPlaySeconds: item.totalPlaySeconds + elapsed, lastPlayedAt: new Date().toISOString() }
          : item,
      );
      await writeLibrary(libraryCache);
    });

    child.unref();
    return { ok: true, message: `Launched ${game.title} with ${modCatalog.summary.enabled} saved enabled mod choices. Playtime tracking started.` };
  } catch (error) {
    return { ok: false, message: error instanceof Error ? error.message : "Launch failed." };
  }
});
