import { promises as fs } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { buildModCatalog, readModState, setModActivation } from "../dist-electron/electron/mods.js";
import { scanInstalledGames } from "../dist-electron/electron/scanner.js";

const root = path.dirname(path.dirname(fileURLToPath(import.meta.url)));
const dataDir = path.join(root, ".local-data");
const libraryFile = path.join(dataDir, "library.json");
const modStateFile = path.join(dataDir, "mods.json");

const originalRaw = await readRawIfExists(modStateFile);

try {
  const games = await readLibraryOrScan();
  const game = games.find((item) => item?.id && item?.title);
  if (!game) throw new Error("No games available for mod persistence verification.");

  const initialCatalog = buildModCatalog(game, await readModState(modStateFile));
  const [enabledMod, disabledMod] = initialCatalog.mods;
  if (!enabledMod || !disabledMod) throw new Error(`Need at least two mod entries for ${game.title}.`);

  await setModActivation(modStateFile, game, enabledMod.id, true);
  await setModActivation(modStateFile, game, disabledMod.id, false);

  const freshState = await readModState(modStateFile);
  const freshCatalog = buildModCatalog(game, freshState);
  const reloadedEnabled = freshCatalog.mods.find((mod) => mod.id === enabledMod.id);
  const reloadedDisabled = freshCatalog.mods.find((mod) => mod.id === disabledMod.id);

  assert(reloadedEnabled?.activationState === "enabled", `${enabledMod.title} did not reload as enabled.`);
  assert(reloadedDisabled?.activationState === "disabled", `${disabledMod.title} did not reload as disabled.`);
  assert(Boolean(reloadedEnabled.lastChangedAt), `${enabledMod.title} missing persisted timestamp.`);
  assert(Boolean(reloadedDisabled.lastChangedAt), `${disabledMod.title} missing persisted timestamp.`);

  await restoreOriginalState();
  const restoredOriginalState = await isOriginalStateRestored();

  console.log(
    JSON.stringify(
      {
        ok: true,
        game: game.title,
        testedMods: [
          { id: enabledMod.id, title: enabledMod.title, expected: "enabled", actual: reloadedEnabled.activationState },
          { id: disabledMod.id, title: disabledMod.title, expected: "disabled", actual: reloadedDisabled.activationState },
        ],
        persistedEnabled: true,
        persistedDisabled: true,
        restoredOriginalState,
      },
      null,
      2,
    ),
  );

  if (!restoredOriginalState) throw new Error("Regression changed the user's original mod state file.");
} catch (error) {
  await restoreOriginalState();
  throw error;
}

async function readLibraryOrScan() {
  try {
    const games = JSON.parse(await fs.readFile(libraryFile, "utf8"));
    if (Array.isArray(games) && games.length > 0) return games;
  } catch {
    // Fall through to a real scan so the regression can run from a clean checkout.
  }

  const result = await scanInstalledGames();
  await fs.mkdir(dataDir, { recursive: true });
  await fs.writeFile(libraryFile, JSON.stringify(result.games, null, 2), "utf8");
  return result.games;
}

async function readRawIfExists(filePath) {
  try {
    return await fs.readFile(filePath, "utf8");
  } catch (error) {
    if (error?.code === "ENOENT") return null;
    throw error;
  }
}

async function restoreOriginalState() {
  if (originalRaw === null) {
    await fs.unlink(modStateFile).catch((error) => {
      if (error?.code !== "ENOENT") throw error;
    });
    return;
  }

  await fs.mkdir(dataDir, { recursive: true });
  await fs.writeFile(modStateFile, originalRaw, "utf8");
}

async function isOriginalStateRestored() {
  const restoredRaw = await readRawIfExists(modStateFile);
  return restoredRaw === originalRaw;
}

function assert(condition, message) {
  if (!condition) throw new Error(message);
}
