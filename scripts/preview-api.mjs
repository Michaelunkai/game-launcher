import { createServer } from "node:http";
import { promises as fs } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { launchExecutable } from "../dist-electron/electron/launcher.js";
import { buildModCatalog, readModState, setModActivation } from "../dist-electron/electron/mods.js";
import { scanInstalledGames } from "../dist-electron/electron/scanner.js";

const root = path.dirname(path.dirname(fileURLToPath(import.meta.url)));
const dataDir = path.join(root, ".local-data");
const dataFile = path.join(dataDir, "library.json");
const modStateFile = path.join(dataDir, "mods.json");
const activeSessions = new Map();
let libraryCache = await readLibrary();

const server = createServer(async (req, res) => {
  setCors(res);
  if (req.method === "OPTIONS") return sendJson(res, 204, {});

  try {
    const url = new URL(req.url ?? "/", "http://127.0.0.1:5274");
    if (req.method === "GET" && url.pathname === "/api/health") {
      return sendJson(res, 200, { ok: true, mode: "real-native-preview-api", games: libraryCache.length });
    }

    if (req.method === "GET" && url.pathname === "/api/library") {
      return sendJson(res, 200, libraryCache);
    }

    if (req.method === "POST" && url.pathname === "/api/scan") {
      const result = await scanInstalledGames();
      const existing = new Map(libraryCache.map((game) => [game.executablePath.toLowerCase(), game]));
      const merged = result.games.map((game) => ({
        ...game,
        totalPlaySeconds: existing.get(game.executablePath.toLowerCase())?.totalPlaySeconds ?? game.totalPlaySeconds,
        lastPlayedAt: existing.get(game.executablePath.toLowerCase())?.lastPlayedAt,
      }));
      await writeLibrary(merged);
      return sendJson(res, 200, { ...result, games: merged });
    }

    if (req.method === "POST" && url.pathname.startsWith("/api/launch/")) {
      const id = decodeURIComponent(url.pathname.replace("/api/launch/", ""));
      const result = await launchGame(id);
      return sendJson(res, result.ok ? 200 : result.status ?? 422, result);
    }

    if (req.method === "GET" && url.pathname.startsWith("/api/mods/")) {
      const id = decodeURIComponent(url.pathname.replace("/api/mods/", ""));
      const game = libraryCache.find((item) => item.id === id);
      if (!game) return sendJson(res, 404, { ok: false, message: "Game not found" });
      return sendJson(res, 200, buildModCatalog(game, await readModState(modStateFile)));
    }

    if (req.method === "POST" && url.pathname.startsWith("/api/mods/")) {
      const segments = url.pathname.split("/").map(decodeURIComponent);
      const gameId = segments[3];
      const modId = segments[4];
      const action = segments[5];
      if (!gameId || !modId || !["enable", "disable"].includes(action)) {
        return sendJson(res, 400, { ok: false, message: "Invalid mod action route." });
      }
      const game = libraryCache.find((item) => item.id === gameId);
      if (!game) return sendJson(res, 404, { ok: false, message: "Game not found" });
      const enabled = action === "enable";
      const catalog = await setModActivation(modStateFile, game, modId, enabled);
      const mod = catalog.mods.find((item) => item.id === modId);
      return sendJson(res, 200, {
        ok: true,
        message: `${enabled ? "Enabled" : "Disabled"} ${mod?.title ?? "mod"} permanently for ${game.title}.`,
        catalog,
      });
    }

    return sendJson(res, 404, { ok: false, message: "Not found" });
  } catch (error) {
    return sendJson(res, 500, { ok: false, message: error instanceof Error ? error.message : "Unknown API error" });
  }
});

server.listen(5274, "127.0.0.1", () => {
  console.log("GameLauncher real preview API listening on http://127.0.0.1:5274");
});

async function launchGame(gameId) {
  const game = libraryCache.find((item) => item.id === gameId);
  if (!game) return { ok: false, status: 404, message: "Game not found in local library." };

  try {
    const modCatalog = buildModCatalog(game, await readModState(modStateFile));
    const child = await launchExecutable(game);
    activeSessions.set(game.id, Date.now());
    child.once("exit", async () => {
      const startedAt = activeSessions.get(game.id);
      if (!startedAt) return;
      activeSessions.delete(game.id);
      const elapsed = Math.max(0, Math.round((Date.now() - startedAt) / 1000));
      libraryCache = libraryCache.map((item) =>
        item.id === game.id
          ? { ...item, totalPlaySeconds: item.totalPlaySeconds + elapsed, lastPlayedAt: new Date().toISOString() }
          : item,
      );
      await writeLibrary(libraryCache);
    });
    child.unref();
    return { ok: true, message: `Launched ${game.title} with ${modCatalog.summary.enabled} saved enabled mod choices. Real playtime tracking started.` };
  } catch (error) {
    return { ok: false, status: 422, message: error instanceof Error ? error.message : "Launch failed." };
  }
}

async function readLibrary() {
  try {
    return JSON.parse(await fs.readFile(dataFile, "utf8"));
  } catch {
    return [];
  }
}

async function writeLibrary(games) {
  libraryCache = games;
  await fs.mkdir(dataDir, { recursive: true });
  await fs.writeFile(dataFile, JSON.stringify(games, null, 2), "utf8");
}

function sendJson(res, status, body) {
  res.writeHead(status, { "Content-Type": "application/json; charset=utf-8" });
  res.end(status === 204 ? "" : JSON.stringify(body));
}

function setCors(res) {
  res.setHeader("Access-Control-Allow-Origin", "http://127.0.0.1:4173");
  res.setHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  res.setHeader("Access-Control-Allow-Headers", "Content-Type");
}
