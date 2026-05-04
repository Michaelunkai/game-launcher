export type GameSource =
  | "steam"
  | "epic"
  | "gog"
  | "ea"
  | "ubisoft"
  | "battle-net"
  | "xbox"
  | "windows-shortcut"
  | "registry"
  | "heuristic-exe"
  | "manual"
  | "demo";

export type MatchStatus = "verified" | "likely" | "needs-review";

export type GameRecord = {
  id: string;
  title: string;
  executablePath: string;
  installPath: string;
  drive: string;
  source: GameSource;
  externalIds?: {
    steamAppId?: string;
    epicCatalogItemId?: string;
  };
  links: {
    steamStore?: string;
    pcGamingWiki?: string;
    steamWorkshop?: string;
    nexusModsSearch?: string;
    modDbSearch?: string;
  };
  dataSources: string[];
  matchStatus: MatchStatus;
  executableConfidence: number;
  metadataConfidence: number;
  totalPlaySeconds: number;
  lastPlayedAt?: string;
  firstDiscoveredAt: string;
  heroImage: string;
  coverImage: string;
  genres: string[];
  tags: string[];
  developers: string[];
  publishers: string[];
  releaseDate?: string;
  screenshots: string[];
  platforms: string[];
  controllerSupport: boolean;
  cloudSave: boolean;
  modSupport: "native" | "community" | "unknown";
  description: string;
  technicalDetails: {
    executableName: string;
    executableSizeMb?: number;
    lastModifiedAt?: string;
    metadataStatus: string;
    discoveryProof: string[];
  };
};

export type ScanProgress = {
  phase: string;
  checked: number;
  found: number;
};

export type ScanResult = {
  games: GameRecord[];
  progress: ScanProgress[];
  scannedDrives: string[];
  completedAt: string;
};

export type ModProviderId =
  | "steam-workshop"
  | "nexus-mods"
  | "moddb"
  | "modio"
  | "curseforge"
  | "thunderstore"
  | "pcgamingwiki"
  | "wemod"
  | "cheat-engine"
  | "trainer-catalog"
  | "community-fix"
  | "visual-mod";

export type ModInstallSupport =
  | "provider-managed"
  | "api-ready"
  | "adapter-required";

export type ModActivationState = "enabled" | "disabled";

export type ModIntegrationStatus =
  | "verified-provider"
  | "api-connectable"
  | "needs-game-adapter"
  | "reference-only";

export type ModActivationMode =
  | "provider-managed"
  | "saved-preference"
  | "adapter-required";

export type GameMod = {
  id: string;
  gameId: string;
  provider: ModProviderId;
  providerName: string;
  title: string;
  summary: string;
  browseUrl: string;
  installSupport: ModInstallSupport;
  integrationStatus: ModIntegrationStatus;
  activationMode: ModActivationMode;
  activationState: ModActivationState;
  installStrategy: string;
  safetyNotes: string[];
  lastChangedAt?: string;
};

export type GameModCatalog = {
  gameId: string;
  gameTitle: string;
  mods: GameMod[];
  refreshedAt: string;
  summary: {
    totalSources: number;
    enabled: number;
    disabled: number;
    providerManaged: number;
    apiConnectable: number;
    adapterRequired: number;
    referenceOnly: number;
    verifiedProvider: number;
    needsGameAdapter: number;
  };
};

export type LauncherApi = {
  getLibrary: () => Promise<GameRecord[]>;
  scanGames: () => Promise<ScanResult>;
  launchGame: (gameId: string) => Promise<{ ok: boolean; message: string }>;
  getMods: (gameId: string) => Promise<GameModCatalog>;
  setModEnabled: (gameId: string, modId: string, enabled: boolean) => Promise<{ ok: boolean; message: string; catalog?: GameModCatalog }>;
};
