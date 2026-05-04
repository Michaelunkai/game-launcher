import { access, mkdir, readFile, writeFile } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { launchExecutable, validateLaunchTarget } from "../dist-electron/electron/launcher.js";
import { buildModCatalog, readModState, setModActivation } from "../dist-electron/electron/mods.js";
import { scanInstalledGames } from "../dist-electron/electron/scanner.js";

const root = path.dirname(path.dirname(fileURLToPath(import.meta.url)));
const dataDir = path.join(root, ".local-data");
const libraryFile = path.join(dataDir, "library.json");
const modStateFile = path.join(dataDir, "mods.json");
const originalModState = await readRawIfExists(modStateFile);

const checks = [];

try {
  const games = await readLibraryOrScan();
  assert(Array.isArray(games), "Library must be an array.");
  assert(games.length >= 40, `Expected at least 40 discovered games, found ${games.length}.`);

  const uniqueIds = new Set(games.map((game) => game.id));
  const uniqueExecutables = new Set(games.map((game) => game.executablePath.toLowerCase()));
  assert(uniqueIds.size === games.length, "Game IDs must be unique.");
  assert(uniqueExecutables.size === games.length, "Executable paths must be unique.");
  checks.push({ name: "library-integrity", games: games.length, uniqueIds: uniqueIds.size, uniqueExecutables: uniqueExecutables.size });

  const missingExecutables = [];
  const missingInstallFolders = [];
  for (const game of games) {
    if (!(await exists(game.executablePath))) missingExecutables.push(game.title);
    if (!(await exists(game.installPath))) missingInstallFolders.push(game.title);
  }
  assert(missingExecutables.length === 0, `Missing executables: ${missingExecutables.join(", ")}`);
  assert(missingInstallFolders.length === 0, `Missing install folders: ${missingInstallFolders.join(", ")}`);
  checks.push({ name: "launch-targets-exist", missingExecutables: 0, missingInstallFolders: 0 });

  const state = await readModState(modStateFile);
  const catalogs = games.map((game) => buildModCatalog(game, state));
  const undersizedCatalogs = catalogs.filter((catalog) => catalog.summary.totalSources < 5);
  const badSummaries = catalogs.filter((catalog) => catalog.summary.enabled + catalog.summary.disabled !== catalog.summary.totalSources);
  const badIntegrationSummaries = catalogs.filter((catalog) => {
    const integrationTotal = catalog.summary.verifiedProvider + catalog.summary.apiConnectable + catalog.summary.needsGameAdapter + catalog.summary.referenceOnly;
    return integrationTotal !== catalog.summary.totalSources;
  });
  const duplicateModIds = catalogs.filter((catalog) => new Set(catalog.mods.map((mod) => mod.id)).size !== catalog.mods.length);
  const missingEvidenceMods = catalogs.flatMap((catalog) =>
    catalog.mods
      .filter((mod) => !mod.browseUrl || !mod.installStrategy || !Array.isArray(mod.safetyNotes) || mod.safetyNotes.length === 0 || !mod.integrationStatus || !mod.activationMode)
      .map((mod) => `${catalog.gameTitle} -> ${mod.title}`),
  );
  const falseWorkingClaims = catalogs.flatMap((catalog) =>
    catalog.mods
      .filter((mod) => mod.integrationStatus === "needs-game-adapter")
      .filter((mod) => /guaranteed|works perfectly|working in-game|automatically applied/i.test(`${mod.title} ${mod.summary} ${mod.installStrategy}`))
      .map((mod) => `${catalog.gameTitle} -> ${mod.title}`),
  );
  const adapterMissingMods = catalogs.flatMap((catalog) =>
    catalog.mods
      .filter((mod) => mod.installSupport === "adapter-required" || mod.integrationStatus === "needs-game-adapter" || mod.activationMode === "adapter-required")
      .map((mod) => `${catalog.gameTitle} -> ${mod.title}`),
  );
  assert(undersizedCatalogs.length === 0, `Mod catalogs too small: ${undersizedCatalogs.map((catalog) => catalog.gameTitle).join(", ")}`);
  assert(badSummaries.length === 0, `Mod summaries do not add up: ${badSummaries.map((catalog) => catalog.gameTitle).join(", ")}`);
  assert(badIntegrationSummaries.length === 0, `Mod integration summaries do not add up: ${badIntegrationSummaries.map((catalog) => catalog.gameTitle).join(", ")}`);
  assert(duplicateModIds.length === 0, `Duplicate mod IDs: ${duplicateModIds.map((catalog) => catalog.gameTitle).join(", ")}`);
  assert(missingEvidenceMods.length === 0, `Mods missing actionable evidence: ${missingEvidenceMods.join(", ")}`);
  assert(falseWorkingClaims.length === 0, `Adapter-required mods falsely claim working status: ${falseWorkingClaims.join(", ")}`);
  assert(adapterMissingMods.length === 0, `Adapter-missing mods must not appear in ready mod catalogs: ${adapterMissingMods.join(", ")}`);
  checks.push({ name: "mod-catalog-integrity", catalogs: catalogs.length, totalMods: catalogs.reduce((sum, catalog) => sum + catalog.summary.totalSources, 0) });
  checks.push({
    name: "mod-readiness",
    providerOrApiRoutes: catalogs.reduce((sum, catalog) => sum + catalog.summary.verifiedProvider + catalog.summary.apiConnectable, 0),
    needsGameAdapter: catalogs.reduce((sum, catalog) => sum + catalog.summary.needsGameAdapter, 0),
    adapterRequired: catalogs.reduce((sum, catalog) => sum + catalog.summary.adapterRequired, 0),
    referenceOnly: catalogs.reduce((sum, catalog) => sum + catalog.summary.referenceOnly, 0),
  });

  const testGame = games[0];
  const testCatalog = buildModCatalog(testGame, await readModState(modStateFile));
  await expectRejects(
    () => setModActivation(modStateFile, testGame, `${testGame.id}:definitely-not-a-real-mod`, true),
    "Unknown mod IDs must be rejected.",
  );
  checks.push({ name: "invalid-mod-action-rejected", game: testGame.title });

  const fakeMissingExeGame = {
    ...testGame,
    id: "backend-regression-missing-exe",
    title: "Backend Regression Missing EXE",
    executablePath: path.join(dataDir, "missing", "MissingGame.exe"),
    installPath: dataDir,
  };
  const missingExeValidation = await validateLaunchTarget(fakeMissingExeGame);
  assert(!missingExeValidation.ok && missingExeValidation.message.includes("executable does not exist"), "Missing EXE must fail validation before spawn.");

  await expectRejects(
    () => launchExecutable(fakeMissingExeGame),
    "Missing EXE launch must reject before claiming success.",
  );
  checks.push({ name: "launch-validation-rejects-missing-exe", ok: true });

  await restoreOriginalModState();
  const restoredOriginalModState = (await readRawIfExists(modStateFile)) === originalModState;
  assert(restoredOriginalModState, "Backend regression must restore original mod state.");

  console.log(JSON.stringify({ ok: true, checks, restoredOriginalModState }, null, 2));
} catch (error) {
  await restoreOriginalModState();
  throw error;
}

async function readLibraryOrScan() {
  try {
    const games = JSON.parse(await readFile(libraryFile, "utf8"));
    if (Array.isArray(games) && games.length > 0) return games;
  } catch {
    // Fall through to a real scanner call.
  }

  const result = await scanInstalledGames();
  await mkdir(dataDir, { recursive: true });
  await writeFile(libraryFile, JSON.stringify(result.games, null, 2), "utf8");
  return result.games;
}

async function exists(filePath) {
  try {
    await access(filePath);
    return true;
  } catch {
    return false;
  }
}

async function readRawIfExists(filePath) {
  try {
    return await readFile(filePath, "utf8");
  } catch (error) {
    if (error?.code === "ENOENT") return null;
    throw error;
  }
}

async function restoreOriginalModState() {
  if (originalModState === null) return;
  await mkdir(dataDir, { recursive: true });
  await writeFile(modStateFile, originalModState, "utf8");
}

async function expectRejects(action, message) {
  try {
    await action();
  } catch {
    return;
  }
  throw new Error(message);
}

function assert(condition, message) {
  if (!condition) throw new Error(message);
}
