import { contextBridge, ipcRenderer } from "electron";
import type { LauncherApi } from "../shared/types.js";

const api: LauncherApi = {
  getLibrary: () => ipcRenderer.invoke("library:get"),
  scanGames: () => ipcRenderer.invoke("library:scan"),
  launchGame: (gameId: string) => ipcRenderer.invoke("game:launch", gameId),
  getMods: (gameId: string) => ipcRenderer.invoke("mods:get", gameId),
  setModEnabled: (gameId: string, modId: string, enabled: boolean) => ipcRenderer.invoke("mods:set-enabled", gameId, modId, enabled),
};

contextBridge.exposeInMainWorld("gameLauncher", api);
