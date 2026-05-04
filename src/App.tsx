import {
  Activity,
  Clock3,
  DatabaseZap,
  Disc3,
  ExternalLink,
  Gamepad2,
  HardDrive,
  LibraryBig,
  MonitorPlay,
  Play,
  Power,
  Puzzle,
  Radar,
  Search,
  ShieldCheck,
  SlidersHorizontal,
  Sparkles,
  Star,
} from "lucide-react";
import { useDeferredValue, useEffect, useRef, useState, useTransition } from "react";
import type { GameModCatalog, GameRecord, LauncherApi, ScanProgress } from "../shared/types";
import { demoApi } from "./demoApi";

const api: LauncherApi = window.gameLauncher ?? demoApi;
type ActiveView = "library" | "recent" | "favorites" | "mods" | "settings";
type LibraryFilter = "all" | "favorites" | "played" | "enabled-mods" | "needs-review";
type LibrarySort = "title" | "recent" | "playtime" | "confidence";
type ModCatalogsByGame = Record<string, GameModCatalog>;

const navItems: Array<{ id: ActiveView; label: string }> = [
  { id: "library", label: "Library" },
  { id: "recent", label: "Recently Played" },
  { id: "favorites", label: "Favorites" },
  { id: "mods", label: "Mods" },
  { id: "settings", label: "Settings" },
];

const CONTINUOUS_SCAN_INTERVAL_MS = 5 * 60 * 1000;
const RETURN_SCAN_THROTTLE_MS = 60 * 1000;
const FAVORITES_STORAGE_KEY = "vaultplay.favoriteGameIds.v1";

const libraryFilters: Array<{ id: LibraryFilter; label: string }> = [
  { id: "all", label: "All" },
  { id: "favorites", label: "Favorites" },
  { id: "played", label: "Played" },
  { id: "enabled-mods", label: "Mods on" },
  { id: "needs-review", label: "Needs review" },
];

const librarySorts: Array<{ id: LibrarySort; label: string }> = [
  { id: "title", label: "Title A-Z" },
  { id: "recent", label: "Recent first" },
  { id: "playtime", label: "Most played" },
  { id: "confidence", label: "Best EXE match" },
];

export default function App() {
  const [games, setGames] = useState<GameRecord[]>([]);
  const [selectedId, setSelectedId] = useState<string>();
  const [activeView, setActiveView] = useState<ActiveView>("library");
  const [query, setQuery] = useState("");
  const deferredQuery = useDeferredValue(query);
  const [filterMode, setFilterMode] = useState<LibraryFilter>("all");
  const [sortMode, setSortMode] = useState<LibrarySort>("title");
  const [scanProgress, setScanProgress] = useState<ScanProgress[]>([]);
  const [modCatalog, setModCatalog] = useState<GameModCatalog>();
  const [allModCatalogs, setAllModCatalogs] = useState<ModCatalogsByGame>({});
  const [favoriteIds, setFavoriteIds] = useState<string[]>(() => {
    try {
      const parsed = JSON.parse(window.localStorage.getItem(FAVORITES_STORAGE_KEY) ?? "[]");
      return Array.isArray(parsed) ? parsed.filter((value): value is string => typeof value === "string") : [];
    } catch {
      return [];
    }
  });
  const [status, setStatus] = useState("Ready to scan every drive with the real native preview API.");
  const [lastScanAt, setLastScanAt] = useState<string>();
  const [scanMode, setScanMode] = useState("Startup full scan pending");
  const [isScanning, setIsScanning] = useState(false);
  const [isLoadingMods, setIsLoadingMods] = useState(false);
  const [isPending, startTransition] = useTransition();
  const autoScanStarted = useRef(false);
  const isScanningRef = useRef(false);
  const lastScanAtRef = useRef<number>(0);

  useEffect(() => {
    void api.getLibrary().then((library) => {
      const seeded = library.length ? library : [];
      setGames(seeded);
      setSelectedId(seeded[0]?.id);
      if (seeded.length) {
        const enrichedCount = seeded.filter((game) => game.dataSources.includes("Steam Store metadata")).length;
        setScanProgress([
          { phase: "Persisted real library loaded", checked: seeded.length, found: seeded.length },
          { phase: "Provider metadata already attached", checked: seeded.length, found: enrichedCount },
        ]);
        setStatus(`Loaded ${seeded.length} real local games; ${enrichedCount} already enriched with provider metadata.`);
      }
      if (!autoScanStarted.current) {
        autoScanStarted.current = true;
        void scan("startup");
      }
    });
  }, []);

  useEffect(() => {
    if (!selectedId && games[0]) setSelectedId(games[0].id);
  }, [games, selectedId]);

  useEffect(() => {
    const timer = window.setInterval(() => {
      if (isScanning) return;
      void api.getLibrary().then((library) => {
        if (library.length) setGames(library);
      });
    }, 15000);
    return () => window.clearInterval(timer);
  }, [isScanning]);

  useEffect(() => {
    const timer = window.setInterval(() => {
      if (!isScanningRef.current) void scan("continuous");
    }, CONTINUOUS_SCAN_INTERVAL_MS);
    return () => window.clearInterval(timer);
  }, []);

  useEffect(() => {
    function scanWhenReturning() {
      if (document.hidden || isScanningRef.current) return;
      if (Date.now() - lastScanAtRef.current < RETURN_SCAN_THROTTLE_MS) return;
      void scan("resume");
    }

    window.addEventListener("focus", scanWhenReturning);
    document.addEventListener("visibilitychange", scanWhenReturning);
    return () => {
      window.removeEventListener("focus", scanWhenReturning);
      document.removeEventListener("visibilitychange", scanWhenReturning);
    };
  }, []);

  useEffect(() => {
    if (!selectedId) {
      setModCatalog(undefined);
      return;
    }
    void api.getMods(selectedId).then(setModCatalog).catch(() => setModCatalog(undefined));
  }, [selectedId]);

  useEffect(() => {
    if ((activeView !== "mods" && filterMode !== "enabled-mods") || !games.length) return;
    const missing = games.filter((game) => !allModCatalogs[game.id]);
    if (!missing.length) return;

    setIsLoadingMods(true);
    void Promise.all(missing.map((game) => api.getMods(game.id)))
      .then((catalogs) => {
        setAllModCatalogs((current) => {
          const next = { ...current };
          for (const catalog of catalogs) next[catalog.gameId] = catalog;
          return next;
        });
        const modCount = catalogs.reduce((sum, catalog) => sum + catalog.summary.totalSources, 0);
        setStatus(`Loaded ${modCount} real mod/trainer/fix entries for ${missing.length} games.`);
      })
      .catch((error) => {
        setStatus(error instanceof Error ? `Real mod loading failed: ${error.message}` : "Real mod loading failed.");
      })
      .finally(() => setIsLoadingMods(false));
  }, [activeView, allModCatalogs, filterMode, games]);

  useEffect(() => {
    window.localStorage.setItem(FAVORITES_STORAGE_KEY, JSON.stringify(favoriteIds));
  }, [favoriteIds]);

  const selected = games.find((game) => game.id === selectedId) ?? games[0];
  const favoriteSet = new Set(favoriteIds);
  const searched = games.filter((game) =>
    `${game.title} ${game.drive} ${game.executablePath} ${game.genres.join(" ")} ${game.tags.join(" ")} ${game.dataSources.join(" ")}`
      .toLowerCase()
      .includes(deferredQuery.toLowerCase()),
  );
  const filtered = sortGames(
    searched.filter((game) => {
      if (filterMode === "favorites") return favoriteSet.has(game.id);
      if (filterMode === "played") return game.totalPlaySeconds > 0 || Boolean(game.lastPlayedAt);
      if (filterMode === "enabled-mods") return (allModCatalogs[game.id]?.summary.enabled ?? 0) > 0;
      if (filterMode === "needs-review") return game.matchStatus === "needs-review" || game.executableConfidence < 84;
      return true;
    }),
    sortMode,
  );
  const favoriteGames = sortGames(games.filter((game) => favoriteSet.has(game.id)), sortMode);
  const recentlyPlayed = [...filtered]
    .filter((game) => game.totalPlaySeconds > 0 || game.lastPlayedAt)
    .sort((a, b) => {
      const lastPlayedDelta = Date.parse(b.lastPlayedAt ?? "") - Date.parse(a.lastPlayedAt ?? "");
      if (!Number.isNaN(lastPlayedDelta) && lastPlayedDelta !== 0) return lastPlayedDelta;
      return b.totalPlaySeconds - a.totalPlaySeconds;
    });
  const modCatalogs = filtered
    .map((game) => ({ game, catalog: allModCatalogs[game.id] }))
    .filter((entry): entry is { game: GameRecord; catalog: GameModCatalog } => Boolean(entry.catalog));
  const globalModSummary = Object.values(allModCatalogs).reduce(
    (summary, catalog) => ({
      enabled: summary.enabled + catalog.summary.enabled,
      totalSources: summary.totalSources + catalog.summary.totalSources,
      verifiedProvider: summary.verifiedProvider + catalog.summary.verifiedProvider,
      apiConnectable: summary.apiConnectable + catalog.summary.apiConnectable,
      needsGameAdapter: summary.needsGameAdapter + catalog.summary.needsGameAdapter,
      referenceOnly: summary.referenceOnly + catalog.summary.referenceOnly,
    }),
    { enabled: 0, totalSources: 0, verifiedProvider: 0, apiConnectable: 0, needsGameAdapter: 0, referenceOnly: 0 },
  );
  const enrichedCount = games.filter((game) => game.dataSources.includes("Steam Store metadata")).length;
  const playableCount = games.filter((game) => game.executableConfidence >= 70).length;
  const verifiedCount = games.filter((game) => game.matchStatus === "verified" || game.executableConfidence >= 84).length;
  const enabledMods = activeView === "mods" ? globalModSummary.enabled : modCatalog?.summary.enabled ?? 0;
  const modSources = activeView === "mods" && globalModSummary.totalSources > 0 ? globalModSummary.totalSources : modCatalog?.summary.totalSources ?? 0;
  const verifiedModPaths =
    activeView === "mods" ? globalModSummary.verifiedProvider + globalModSummary.apiConnectable : (modCatalog?.summary.verifiedProvider ?? 0) + (modCatalog?.summary.apiConnectable ?? 0);
  const busy = isScanning || isPending || isLoadingMods;

  async function scan(reason: "manual" | "startup" | "continuous" | "resume" = "manual") {
    if (isScanning) return;
    if (isScanningRef.current) return;
    isScanningRef.current = true;
    setIsScanning(true);
    setScanMode(reasonLabel(reason));
    setStatus(
      reason === "startup"
        ? "Startup auto-scan running: checking drives, launchers, registry, shortcuts, EXE confidence, and metadata..."
        : reason === "continuous"
          ? "Continuous discovery scan running in the background across all fixed drives and launcher sources..."
          : reason === "resume"
            ? "Return-to-app scan running: checking for games added while VaultPlay was open..."
        : "Scanning all fixed drives, store manifests, registry entries, shortcuts, and game-like executable paths...",
    );
    try {
      const result = await api.scanGames();
      startTransition(() => {
        setGames(result.games);
        setSelectedId((current) => current && result.games.some((game) => game.id === current) ? current : result.games[0]?.id);
        setAllModCatalogs({});
        setScanProgress(result.progress);
        const enriched = result.games.filter((game) => game.dataSources.includes("Steam Store metadata")).length;
        setLastScanAt(result.completedAt);
        lastScanAtRef.current = Date.parse(result.completedAt);
        setScanMode(`Last full scan: ${new Date(result.completedAt).toLocaleTimeString()}`);
        setStatus(`Scan complete: ${result.games.length} playable games across ${result.scannedDrives.join(", ")}; ${enriched} enriched with provider data.`);
      });
    } catch (error) {
      setStatus(error instanceof Error ? `Scan failed: ${error.message}` : "Scan failed with an unknown error.");
    } finally {
      isScanningRef.current = false;
      setIsScanning(false);
    }
  }

  async function launchGameWithSavedMods(game: GameRecord) {
    const catalog = allModCatalogs[game.id] ?? (modCatalog?.gameId === game.id ? modCatalog : undefined);
    setSelectedId(game.id);
    setStatus(`Launching ${game.title} with ${catalog?.summary.enabled ?? 0} saved mod preferences; provider/API mods apply through their validated route.`);
    const result = await api.launchGame(game.id);
    setStatus(result.message);
  }

  function toggleFavorite(gameId: string) {
    setFavoriteIds((current) => current.includes(gameId) ? current.filter((id) => id !== gameId) : [...current, gameId]);
  }

  async function toggleMod(gameId: string, modId: string, enabled: boolean) {
    const game = games.find((item) => item.id === gameId);
    if (!game) return;
    const result = await api.setModEnabled(game.id, modId, enabled);
    if (result.catalog) {
      const catalog = result.catalog;
      if (selected?.id === game.id) setModCatalog(catalog);
      setAllModCatalogs((current) => ({ ...current, [game.id]: catalog }));
    }
    const savedWord = enabled ? "Saved" : "Removed";
    setStatus(`${savedWord} mod preference permanently. ${result.message}`);
  }

  function selectView(view: ActiveView) {
    setActiveView(view);
    if (view === "mods") setStatus("Loading every discovered game's mod sources, saved preferences, and verified integration status.");
    if (view === "recent") setStatus("Showing games with real tracked launcher playtime.");
  }

  return (
    <main className="app-shell">
      <div className="ambient ambient-one" />
      <div className="ambient ambient-two" />
      <aside className="sidebar">
        <div className="brand">
          <div className="brand-mark">
            <Disc3 size={22} />
          </div>
          <div>
            <strong>VaultPlay</strong>
            <span>Universal PC launcher</span>
          </div>
        </div>

        <button className="scan-button" onClick={() => scan()} disabled={busy}>
          <Radar size={18} />
          {busy ? "Scanning..." : "Auto-scan all drives"}
        </button>

        <nav className="nav-list">
          {navItems.map((item) => (
            <button className={activeView === item.id ? "active" : ""} key={item.id} onClick={() => selectView(item.id)}>
              {item.label}
            </button>
          ))}
        </nav>

        <section className="side-card">
          <span>Discovery contract</span>
          <p>Find every possible game source, score the correct EXE, enrich artwork/data, and learn from manual corrections.</p>
        </section>
      </aside>

      <section className="stage">
        <header className="topbar">
          <label className="search">
            <Search size={18} />
            <input value={query} onChange={(event) => setQuery(event.target.value)} placeholder="Search games, drives, tags, paths..." />
          </label>
          <div className="status-line">
            <ShieldCheck size={17} />
            <span>{status}</span>
          </div>
        </header>

        <section className="library-command">
          <StatCard icon={<LibraryBig size={18} />} label="Real games found" value={String(games.length)} subvalue="Auto-scanned on every launch" />
          <StatCard icon={<MonitorPlay size={18} />} label="Playable EXEs" value={`${playableCount}/${games.length || 0}`} subvalue={`${verifiedCount} high confidence`} />
          <StatCard icon={<DatabaseZap size={18} />} label="Provider enriched" value={String(enrichedCount)} subvalue="Steam data, art, dates, genres" />
          <StatCard icon={<Puzzle size={18} />} label="Saved mod choices" value={`${enabledMods}/${modSources}`} subvalue={`${verifiedModPaths} provider/API routes`} />
        </section>

        <section className="coverage-strip" aria-label="Continuous discovery coverage">
          <div>
            <strong>Continuous discovery is active</strong>
            <span>Startup scan, manual scan, return-to-app scan, and a full background rescan every 5 minutes.</span>
          </div>
          <div>
            <strong>{scanMode}</strong>
            <span>{lastScanAt ? `Freshness proof: ${new Date(lastScanAt).toLocaleString()}` : "Waiting for first completed full scan"}</span>
          </div>
        </section>

        <section className="library-tools" aria-label="Library filters and sorting">
          <div className="tool-copy">
            <SlidersHorizontal size={18} />
            <span><strong>{filtered.length}</strong> shown from {games.length} discovered games</span>
          </div>
          <div className="filter-pills">
            {libraryFilters.map((item) => (
              <button className={filterMode === item.id ? "active" : ""} key={item.id} onClick={() => setFilterMode(item.id)}>
                {item.label}
              </button>
            ))}
          </div>
          <label className="sort-select">
            Sort
            <select value={sortMode} onChange={(event) => setSortMode(event.target.value as LibrarySort)}>
              {librarySorts.map((item) => (
                <option key={item.id} value={item.id}>{item.label}</option>
              ))}
            </select>
          </label>
        </section>

        {activeView === "library" && selected ? (
          <section className="hero" style={{ backgroundImage: `linear-gradient(90deg, rgba(5,7,13,.96), rgba(5,7,13,.68), rgba(5,7,13,.2)), url("${selected.heroImage}")` }}>
            <div className="hero-copy">
              <span className={`match ${selected.matchStatus}`}>{selected.matchStatus.replace("-", " ")}</span>
              <h1>{selected.title}</h1>
              <p>{selected.description}</p>
              <div className="hero-actions">
                <button className="play-button" onClick={() => launchGameWithSavedMods(selected)}>
                  <Play size={22} fill="currentColor" />
                  Play
                </button>
                <button className={`favorite-button ${favoriteSet.has(selected.id) ? "active" : ""}`} onClick={() => toggleFavorite(selected.id)}>
                  <Star size={18} fill={favoriteSet.has(selected.id) ? "currentColor" : "none"} />
                  {favoriteSet.has(selected.id) ? "Favorite" : "Add favorite"}
                </button>
                <button className="ghost-button" onClick={() => scan()}>
                  Rescan library
                </button>
              </div>
              <div className="hero-meta-strip">
                {[selected.source, selected.modSupport, ...selected.genres.slice(0, 2), ...selected.tags.slice(0, 3)].map((item) => (
                  <span key={item}>{item}</span>
                ))}
              </div>
            </div>

            <div className="detail-panel">
              <Metric icon={<HardDrive size={17} />} label="Install drive" value={selected.drive} />
              <Metric icon={<Activity size={17} />} label="Correct EXE" value={`${selected.executableConfidence}%`} />
              <Metric icon={<DatabaseZap size={17} />} label="Metadata match" value={`${selected.metadataConfidence}%`} />
              <Metric icon={<Clock3 size={17} />} label="Total played" value={formatTime(selected.totalPlaySeconds)} />
              <div className="path-box">{selected.executablePath}</div>
              <div className="path-box">{selected.technicalDetails.metadataStatus}</div>
              <div className="proof-stack">
                {(selected.technicalDetails.discoveryProof ?? []).map((proof) => (
                  <span key={proof}>{proof}</span>
                ))}
              </div>
            </div>
          </section>
        ) : activeView === "library" ? (
          <section className="empty-hero">
            <h1>Your universal gaming library starts here.</h1>
            <p>Run the scanner to discover games across every fixed drive and launcher source.</p>
            <button className="play-button" onClick={() => scan()}>
              <Radar size={20} />
              Start full PC scan
            </button>
          </section>
        ) : null}

        {activeView === "library" && (
          <>
            <section className="coverflow" aria-label="Discovered game cover flow">
              {filtered.map((game, index) => (
                <button
                  className={`cover-card ${game.id === selected?.id ? "selected" : ""}`}
                  key={game.id}
                  onClick={() => setSelectedId(game.id)}
                  style={{ "--tilt": `${(index - 2) * 4}deg` } as React.CSSProperties}
                >
                  <img src={game.coverImage} alt="" />
                  <strong>{game.title}</strong>
                  <span>{game.drive} • {formatTime(game.totalPlaySeconds)}</span>
                </button>
              ))}
            </section>

            <section className="library-grid" aria-label="Useful game library">
              {filtered.slice(0, 18).map((game) => (
                <article className={`library-row ${game.id === selected?.id ? "selected" : ""}`} key={`${game.id}-row`}>
                  <button className="library-row-main" onClick={() => setSelectedId(game.id)}>
                    <img src={game.coverImage} alt="" />
                    <span>
                      <strong>{game.title}</strong>
                      <small>{game.executablePath}</small>
                    </span>
                    <em>{game.executableConfidence}% EXE</em>
                    <em>{game.metadataConfidence}% data</em>
                  </button>
                  <div className="library-row-actions">
                    <button className={favoriteSet.has(game.id) ? "active" : ""} onClick={() => toggleFavorite(game.id)} aria-label={`${favoriteSet.has(game.id) ? "Remove" : "Add"} ${game.title} favorite`}>
                      <Star size={15} fill={favoriteSet.has(game.id) ? "currentColor" : "none"} />
                    </button>
                    <button className="mini-play" onClick={() => launchGameWithSavedMods(game)}>
                      <Play size={14} fill="currentColor" />
                      Play
                    </button>
                  </div>
                </article>
              ))}
            </section>
          </>
        )}

        {activeView === "library" && selected && modCatalog && (
          <section className="mod-center" aria-label="Permanent mod manager">
            <div className="mod-header">
              <div>
                <span>Permanent Mod Center</span>
                <h2>{selected.title} real mods</h2>
                <p>
                  Trainer actions, community mods, and PC fixes are tracked per game. A toggle saves your permanent preference; VaultPlay only marks a mod as automatically applicable when a provider, API route, or game-specific adapter validates it.
                </p>
              </div>
              <div className="mod-summary">
                <strong>{modCatalog.summary.enabled}</strong>
                <span>saved of {modCatalog.summary.totalSources}</span>
              </div>
            </div>

            <div className="mod-grid">
              {modCatalog.mods.map((mod) => (
                <article className={`mod-card ${mod.activationState}`} key={mod.id}>
                  <div className="mod-card-top">
                    <span className={`support ${mod.installSupport}`}>{supportLabel(mod.installSupport)}</span>
                    <strong>{mod.providerName}</strong>
                  </div>
                  <span className={`integration ${mod.integrationStatus}`}>{integrationLabel(mod.integrationStatus)}</span>
                  <h3>{mod.title}</h3>
                  <p>{mod.summary}</p>
                  <div className="mod-strategy">{mod.installStrategy}</div>
                  <div className="mod-safety">{integrationCopy(mod.integrationStatus)}</div>
                  <div className="mod-actions">
                    <button className="mod-toggle" onClick={() => toggleMod(selected.id, mod.id, mod.activationState !== "enabled")}>
                      <Power size={16} />
                      {mod.activationState === "enabled" ? "Remove saved preference" : "Save preference"}
                    </button>
                    <a className="mod-link" href={mod.browseUrl} target="_blank" rel="noreferrer">
                      <ExternalLink size={16} />
                      Browse
                    </a>
                  </div>
                </article>
              ))}
            </div>
          </section>
        )}

        {activeView === "mods" && (
          <section className="all-mods-view" aria-label="All games mod manager">
            <div className="view-hero compact">
              <span>All-game mod control</span>
              <h1>Mods for every discovered game</h1>
              <p>
                VaultPlay lists concrete mod, trainer, cheat-table, and fix actions for every game, saves your choices permanently, and refuses to call a mod working until a provider, API route, or game-specific adapter can validate it.
              </p>
            </div>

            {isLoadingMods && <div className="loading-strip">Loading real mods, trainers, cheats, and fixes across {games.length} games...</div>}

            <div className="all-mods-grid">
              {modCatalogs.map(({ game, catalog }) => (
                <article className="game-mod-panel" key={`${game.id}-mods`}>
                  <div className="game-mod-panel-head">
                    <button className="game-mod-title" onClick={() => setSelectedId(game.id)}>
                      <img src={game.coverImage} alt="" />
                      <span>
                        <strong>{game.title}</strong>
                        <small>{catalog.summary.enabled} saved of {catalog.summary.totalSources} mods · {catalog.summary.verifiedProvider + catalog.summary.apiConnectable} provider/API routes · {game.drive}</small>
                      </span>
                    </button>
                    <button className="panel-play-button" onClick={() => launchGameWithSavedMods(game)} aria-label={`Play ${game.title} with saved mods`}>
                      <Play size={17} fill="currentColor" />
                      PLAY
                    </button>
                  </div>
                  <div className="compact-mod-list">
                    {catalog.mods.map((mod) => (
                      <div className={`compact-mod ${mod.activationState}`} key={mod.id}>
                        <span className={`support ${mod.installSupport}`}>{supportLabel(mod.installSupport)}</span>
                        <strong>{mod.title}</strong>
                        <small>{integrationLabel(mod.integrationStatus)}</small>
                        <button onClick={() => toggleMod(game.id, mod.id, mod.activationState !== "enabled")}>
                          <Power size={14} />
                          {mod.activationState === "enabled" ? "Remove" : "Save"}
                        </button>
                      </div>
                    ))}
                  </div>
                </article>
              ))}
            </div>
          </section>
        )}

        {activeView === "recent" && (
          <section className="recent-view" aria-label="Recently played games">
            <div className="view-hero compact">
              <span>Tracked play history</span>
              <h1>Recently played</h1>
              <p>Only games launched through VaultPlay with measured playtime appear here. Total time is stored per game and updates after the launched process exits.</p>
            </div>

            {recentlyPlayed.length ? (
              <div className="recent-list">
                {recentlyPlayed.map((game, index) => (
                  <button className="recent-row" key={`${game.id}-recent`} onClick={() => setSelectedId(game.id)}>
                    <span className="rank">{String(index + 1).padStart(2, "0")}</span>
                    <img src={game.coverImage} alt="" />
                    <span>
                      <strong>{game.title}</strong>
                      <small>{game.executablePath}</small>
                    </span>
                    <em>{formatTime(game.totalPlaySeconds)} total</em>
                    <em>{formatLastPlayed(game.lastPlayedAt)}</em>
                  </button>
                ))}
              </div>
            ) : (
              <div className="empty-panel">
                <h2>No tracked play sessions yet</h2>
                <p>Press Play from VaultPlay, close the game when finished, and this view will show the correct game, last played time, and total tracked playtime.</p>
              </div>
            )}
          </section>
        )}

        {activeView === "favorites" && (
          <section className="favorites-view" aria-label="Favorite games">
            <div className="view-hero compact">
              <span>Your launch shelf</span>
              <h1>Favorites</h1>
              <p>Keep the games you return to most in one clean, quick-launch shelf. Favorites are saved locally in this browser/app session.</p>
            </div>

            {favoriteGames.length ? (
              <div className="favorite-shelf">
                {favoriteGames.map((game) => (
                  <article className="favorite-card" key={`${game.id}-favorite`}>
                    <img src={game.coverImage} alt="" />
                    <div>
                      <strong>{game.title}</strong>
                      <span>{game.drive} · {formatTime(game.totalPlaySeconds)} played · {game.executableConfidence}% EXE</span>
                    </div>
                    <button onClick={() => launchGameWithSavedMods(game)}>
                      <Play size={16} fill="currentColor" />
                      Play
                    </button>
                    <button className="quiet-icon" onClick={() => toggleFavorite(game.id)} aria-label={`Remove ${game.title} favorite`}>
                      <Star size={16} fill="currentColor" />
                    </button>
                  </article>
                ))}
              </div>
            ) : (
              <div className="empty-panel">
                <h2>No favorites yet</h2>
                <p>Use the star on the hero or any library row to pin games here for faster launching.</p>
              </div>
            )}
          </section>
        )}

        {activeView === "settings" && (
          <section className="settings-view" aria-label="Launcher settings and proof">
            <div className="view-hero compact">
              <span>Launcher proof</span>
              <h1>Settings</h1>
              <p>Operational settings and live scan proof for discovery, metadata, mod state, and playtime tracking.</p>
            </div>
            <div className="settings-grid">
              <div className="settings-card">
                <strong>Discovery cadence</strong>
                <span>Startup scan, manual scan, return-to-app scan, and full background scan every 5 minutes.</span>
              </div>
              <div className="settings-card">
                <strong>Saved mod profile</strong>
                <span>{enabledMods} saved choices across {modSources} loaded mod entries. Saved state is persisted by game and mod id; working status is tracked separately.</span>
              </div>
              <div className="settings-card">
                <strong>Launch safety</strong>
                <span>Play buttons launch the discovered EXE path and track process time. Mod preferences are not presented as working unless a provider/API route or game adapter can safely apply them.</span>
              </div>
              <div className="settings-card">
                <strong>Scan freshness</strong>
                <span>{lastScanAt ? new Date(lastScanAt).toLocaleString() : "Waiting for the next completed scan."}</span>
              </div>
            </div>
          </section>
        )}

        {activeView === "library" && <section className="lower-grid">
          <div className="glass-card wide">
            <div className="section-title">
              <Sparkles size={18} />
              <h2>Automatic enrichment pipeline</h2>
            </div>
            <div className="pipeline">
              {["Drive scan", "Store manifests", "EXE scoring", "Game metadata", "Provider art", "Real mod discovery", "Permanent mod state", "Playtime tracker"].map((step, index) => (
                <div className="pipe-step" key={step}>
                  <span>{String(index + 1).padStart(2, "0")}</span>
                  {step}
                </div>
              ))}
            </div>
          </div>

          <div className="glass-card">
            <div className="section-title">
              <Gamepad2 size={18} />
              <h2>Selected game data</h2>
            </div>
            {selected && (
              <div className="chips">
                {[
                  ...selected.genres,
                  ...selected.tags,
                  ...selected.dataSources,
                  selected.releaseDate ? `Released ${selected.releaseDate}` : "Release date pending",
                  selected.developers.length ? `Dev: ${selected.developers.slice(0, 2).join(", ")}` : "Developer pending",
                  selected.publishers.length ? `Publisher: ${selected.publishers.slice(0, 2).join(", ")}` : "Publisher pending",
                  selected.screenshots.length ? `${selected.screenshots.length} screenshots` : "Screenshots pending",
                  selected.technicalDetails.executableName,
                  selected.technicalDetails.executableSizeMb ? `${selected.technicalDetails.executableSizeMb} MB` : "Size unknown",
                  selected.controllerSupport ? "Controller" : "Keyboard",
                  selected.cloudSave ? "Cloud save" : "Local save",
                  `${selected.modSupport} mods`,
                  selected.links.steamStore ? "Steam store link" : "Steam search pending",
                  selected.links.pcGamingWiki ? "PCGamingWiki search" : "PCGamingWiki pending",
                  selected.links.nexusModsSearch ? "Nexus Mods search" : "Nexus pending",
                  selected.links.modDbSearch ? "ModDB search" : "ModDB pending",
                ].map((chip) => (
                  <span key={chip}>{chip}</span>
                ))}
              </div>
            )}
          </div>

          <div className="glass-card">
            <div className="section-title">
              <Radar size={18} />
              <h2>Latest scan proof</h2>
            </div>
            <div className="scan-log">
              {(scanProgress.length ? scanProgress : [{ phase: "Waiting for first scan", checked: 0, found: games.length }]).map((item) => (
                <p key={item.phase}>
                  <strong>{item.found}</strong> found after {item.checked} checked · {item.phase}
                </p>
              ))}
            </div>
          </div>
        </section>}
      </section>
    </main>
  );
}

function StatCard({ icon, label, value, subvalue }: { icon: React.ReactNode; label: string; value: string; subvalue: string }) {
  return (
    <div className="stat-card">
      <div>{icon}</div>
      <span>{label}</span>
      <strong>{value}</strong>
      <small>{subvalue}</small>
    </div>
  );
}

function supportLabel(value: GameModCatalog["mods"][number]["installSupport"]) {
  if (value === "provider-managed") return "Provider managed";
  if (value === "api-ready") return "API ready";
  return "Adapter required";
}

function integrationLabel(value: GameModCatalog["mods"][number]["integrationStatus"]) {
  if (value === "verified-provider") return "Verified provider route";
  if (value === "api-connectable") return "API connectable";
  if (value === "reference-only") return "Reference only";
  return "Needs game adapter";
}

function integrationCopy(value: GameModCatalog["mods"][number]["integrationStatus"]) {
  if (value === "verified-provider") return "Provider controls install/update; VaultPlay saves the preference and launches the validated route.";
  if (value === "api-connectable") return "Ready for provider API integration once credentials/game IDs are configured; saved now, not blindly installed.";
  if (value === "reference-only") return "Useful compatibility intelligence, not a one-click mod installer.";
  return "Saved as a desired mod only. It is not claimed working until an exact game adapter validates install, dependencies, and safe launch.";
}

function Metric({ icon, label, value }: { icon: React.ReactNode; label: string; value: string }) {
  return (
    <div className="metric">
      {icon}
      <span>{label}</span>
      <strong>{value}</strong>
    </div>
  );
}

function formatTime(seconds: number) {
  const hours = Math.floor(seconds / 3600);
  const minutes = Math.floor((seconds % 3600) / 60);
  if (hours <= 0) return `${minutes}m`;
  return `${hours}h ${minutes}m`;
}

function formatLastPlayed(value?: string) {
  if (!value) return "Not recorded";
  const date = new Date(value);
  if (Number.isNaN(date.getTime())) return "Not recorded";
  return date.toLocaleString(undefined, { dateStyle: "medium", timeStyle: "short" });
}

function sortGames(games: GameRecord[], sortMode: LibrarySort) {
  return [...games].sort((a, b) => {
    if (sortMode === "recent") {
      const aTime = Date.parse(a.lastPlayedAt ?? "");
      const bTime = Date.parse(b.lastPlayedAt ?? "");
      return (Number.isNaN(bTime) ? 0 : bTime) - (Number.isNaN(aTime) ? 0 : aTime);
    }
    if (sortMode === "playtime") return b.totalPlaySeconds - a.totalPlaySeconds;
    if (sortMode === "confidence") return b.executableConfidence - a.executableConfidence || b.metadataConfidence - a.metadataConfidence;
    return a.title.localeCompare(b.title);
  });
}

function reasonLabel(reason: "manual" | "startup" | "continuous" | "resume") {
  if (reason === "startup") return "Startup full scan running";
  if (reason === "continuous") return "Continuous background scan running";
  if (reason === "resume") return "Return-to-app scan running";
  return "Manual full scan running";
}
