import type { ChildProcess } from "node:child_process";
import { spawn } from "node:child_process";
import { promises as fs } from "node:fs";
import type { GameRecord } from "../shared/types.js";

export type LaunchValidationResult = {
  ok: boolean;
  message: string;
};

export async function validateLaunchTarget(game: GameRecord): Promise<LaunchValidationResult> {
  const executable = await pathExists(game.executablePath);
  if (!executable) {
    return { ok: false, message: `Launch blocked: executable does not exist at ${game.executablePath}` };
  }

  const installPath = await pathExists(game.installPath);
  if (!installPath) {
    return { ok: false, message: `Launch blocked: install folder does not exist at ${game.installPath}` };
  }

  return { ok: true, message: "Launch target exists." };
}

export async function launchExecutable(game: GameRecord): Promise<ChildProcess> {
  const validation = await validateLaunchTarget(game);
  if (!validation.ok) throw new Error(validation.message);

  const child = spawn(game.executablePath, [], {
    cwd: game.installPath,
    detached: true,
    stdio: "ignore",
  });

  await waitForSpawn(child);
  return child;
}

async function waitForSpawn(child: ChildProcess): Promise<void> {
  await new Promise<void>((resolve, reject) => {
    const timeout = setTimeout(() => {
      cleanup();
      reject(new Error("Launch failed: Windows did not confirm process start in time."));
    }, 3_000);

    function cleanup() {
      clearTimeout(timeout);
      child.off("spawn", onSpawn);
      child.off("error", onError);
    }

    function onSpawn() {
      cleanup();
      resolve();
    }

    function onError(error: Error) {
      cleanup();
      reject(error);
    }

    child.once("spawn", onSpawn);
    child.once("error", onError);
  });
}

async function pathExists(filePath: string): Promise<boolean> {
  try {
    await fs.access(filePath);
    return true;
  } catch {
    return false;
  }
}
