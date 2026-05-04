import { access } from "node:fs/promises";
import { scanInstalledGames } from "../dist-electron/electron/scanner.js";

const startedAt = Date.now();
const result = await scanInstalledGames();
const games = result.games;
const enriched = games.filter((game) => game.dataSources.includes("Steam Store metadata"));
const falseMatches = games.filter((game) => /QuickSFV|_Redist|[\\/]redist[\\/]/i.test(game.executablePath));
const missingExecutables = [];

for (const game of games) {
  try {
    await access(game.executablePath);
  } catch {
    missingExecutables.push(game);
  }
}

const failures = [];
if (games.length < 40) failures.push(`Expected at least 40 real local games on this machine, found ${games.length}.`);
if (enriched.length < 30) failures.push(`Expected at least 30 provider-enriched games, found ${enriched.length}.`);
if (falseMatches.length) failures.push(`Rejected helper/redist tools leaked into library: ${falseMatches.map((game) => game.executablePath).join("; ")}`);
if (missingExecutables.length) failures.push(`Missing launch EXEs: ${missingExecutables.map((game) => game.executablePath).join("; ")}`);

const summary = {
  ms: Date.now() - startedAt,
  drives: result.scannedDrives,
  total: games.length,
  enriched: enriched.length,
  falseMatches: falseMatches.length,
  missingExecutables: missingExecutables.length,
  progress: result.progress,
};

console.log(JSON.stringify(summary, null, 2));

if (failures.length) {
  console.error(failures.join("\n"));
  process.exit(1);
}
