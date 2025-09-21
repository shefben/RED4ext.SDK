Consolidated TODOs (from analysis docs)

Scope: Unfinished, partially-implemented, or unverified items collected from:
- CRITICAL_BUGS_ANALYSIS.md
- extensive_stub_feature_analysis_results.md
- GAMEPLAY_IMPROVEMENTS.md
- INCOMPLETE_FEATURES_ANALYSIS.md
- scripting_issues.md
- THREAD_SAFETY_ISSUES.md
- UI_IMPROVEMENTS_SUMMARY.md

Notes
- Verified as completed: specific VoiceEncoder buffer bounds, bundle cache locking, Net.cpp null/size checks, core UI screens present. Remaining items here require further work or explicit validation.

Critical/High Priority
- Thread safety (Net.cpp globals) — PARTIAL
  - Protect all access to `g_Host`, `g_Peers`, and `g_nextPeerId` with `g_NetMutex` (many call sites still unguarded).
  - Add shutdown synchronization and operation timeouts.

- Save Game Synchronization — PARTIAL (SaveGameSync.reds, SaveGameManager.*)
  - Persist to real CP2077 save format; remove placeholders.
  - Implement gig completion, discovered locations, vehicle states, world events collection (TODOs present).
  - Complete coordinated load/save flows and conflict resolution.

- Voice System — PARTIAL (VoiceManager.cpp, CoopVoice.reds)
  - Implement Opus encode/decode in VoiceManager (currently placeholders).
  - Add noise suppression/VAD and finalize spatial audio mixing.

- Quest Synchronization — PARTIAL
  - Advanced branching coordination, conflict resolution edge cases, and NPC interaction sync still need validation/fill-ins despite large existing implementation (QuestWatchdog, QuestSync).

Stability/Correctness
- Memory leak prevention — UNVERIFIED
  - Audit dynamic allocations and long-lived buffers (voice, snapshots, asset streaming, HTTP).

- Exception/error handling — PARTIAL
  - Standardize error paths and logging in net/voice/server subsystems.
  - Convert ad-hoc std::cout/err to structured Logger with levels.

- Input/packet validation — PARTIAL
  - Ensure size/bounds checks across all message types; some done in Net.cpp, review remaining packet handlers.

Subsystems & Features
- Inventory synchronization backend — PARTIAL
  - Conflict resolution and cross-player validation present; finish vendor stock sync, crafting/mod attachment flows end-to-end.

- Vehicle physics integration — PARTIAL
  - Improve damage validation, multi-passenger sync, interpolation for high latency, and engine-backed state.

- Performance monitoring — PARTIAL
  - Expand monitoring across platforms; auto-scaling heuristics; memory optimization passes.

- Asset streaming — PARTIAL
  - Integrate with game asset formats; de-hardcode priorities; improve cache invalidation.

- Network security — PARTIAL
  - Encryption coverage, replay prevention (nonce windows done in places), broader rate limiting.

- Database integration — UNIMPLEMENTED
  - Persist player stats, leaderboards, bans, and configurations (choose Redis/Postgres, add schema/migration).

- Content/activity systems — UNIMPLEMENTED/ROADMAP
  - Game modes, events, economy systems, UGC tools from roadmap (GAMEPLAY_IMPROVEMENTS.md) are not implemented.

UI/UX
- UI integration — MOSTLY PRESENT, FURTHER POLISH
  - ModernServerBrowser, ServerHostDialog, CoopUI, MainMenuInjection implemented; continue accessibility, localization, screen reader, and high-contrast mode.

Testing
- Add/expand tests and validation
  - Stress (multi-client), memory (ASan/Valgrind), thread (TSan), and integration test plans.

Recommended Next Steps
- Complete thread-safety pass on networking globals.
- Land Opus pipeline in VoiceManager and remove placeholders.
- Finish SaveGameSync data collection TODOs and persistence glue.
- Implement persistent auth/ban store; tighten packet validations.
- Stand up minimal DB service for persistence-backed features.


Additional Consolidated TODOs (second pass)

- Deep Analysis outstanding (selected, unverified or partial):
  - AssetStreamer.cpp: audit buffer usage and lifetimes for overflow risks.
  - GameClock.cpp: address identified thread-safety concerns.
  - GameProcess.cpp: sanitize command invocations; remove injection vectors.
  - SaveFork.cpp/SaveMigration.cpp: validate buffers/format parsing; fix path handling.
  - InterestGrid global instance: ensure thread-safe access.
  - NatClient/NatTraversal: secure credential storage; add timeouts and resource cleanup.
  - StatBatch.cpp: remove hardcoded endpoints; add validation.
  - LedgerService/InventoryController: fix races and potential leaks; re-review locking.
  - WebDash/InfoServer: sanitize HTTP inputs; platform guards for sockets.
  - VoiceDecoder.cpp: thread safety and sequence wraparound handling.
  - Vehicle/CarPhysics: resolve any undefined calls; align with deterministic step.
  - CarryController/Journal: avoid empty/busy updates; batch or keep handle open.

- Network implementation gaps (vs. docs):
  - Ensure ban list persists (current checks appear stubbed).

- Voice Chat completeness:
  - Implement Opus encode/decode in VoiceManager (currently placeholder), add VAD/noise suppression.

- InventorySync.reds placeholders:
  - Replace temporary GetPlayerPeerId/Net_SendInventorySnapshot shortcuts with real natives.
  - Complete database-backed validation pathways referenced by InventoryDatabaseManager.

- Phase Plan coverage:
  - Review tickets P0–P8; mark DONE where implemented and schedule remaining (delta-snapshot polish, rollback buffers, game mode rules, world sync polish, threading/telemetry/version checks).


Detailed Implementation Notes (per TODO)

- Networking Thread Safety — PARTIAL
  - Unguarded globals: `g_Host`, `g_Peers`, `g_nextPeerId` are defined in `src/net/Net.cpp` (lines ~217–219) with `g_NetMutex` declared (line ~223) but only used in `Net_Init()` (lock at ~236). All other access sites are unguarded.
  - Unguarded writers/readers (non-exhaustive):
    - `Net_Shutdown()` clears `g_Peers` and destroys `g_Host` without a lock (~251–261).
    - `Net_Poll()` reads/writes peers, assigns `g_nextPeerId++`, pushes/erases from `g_Peers` (~297, 310, 341), scans on receive (~367ff) — no locks.
    - Connection utilities iterate `g_Peers` in many spots (search results ~429–1709).
    - `Net_StartServer()` destroys/re-creates `g_Host` (~1889–1894) without a lock.
    - `Net_ConnectToServer()` reads `g_Host` and creates a peer (~1927–1936) without a lock.
  - Fix pattern: wrap every read/write of these globals in `std::lock_guard<std::mutex> lock(g_NetMutex);` (or a finer-grained mutex split if contention becomes an issue). For iterations that call into other functions, keep the lock’s critical section minimal.
  - Add shutdown synchronization: ensure no threads can call `enet_host_service()` after `Net_Shutdown()` begins; gate `Net_Poll()` with an atomic `g_running` flag protected by the same mutex.

- Save Game Synchronization — PARTIAL (SaveGameSync.reds, SaveGameManager.*)
  - Outstanding TODOs in `src/save/SaveGameSync.reds`:
    - Proper time/weather/world integration: lines ~561, ~573, ~585, ~602, ~619, ~636, ~646 show placeholders and error logging when subsystems are missing.
    - Data gaps to implement:
      - Completed gigs collection at ~594: iterate quest database for “Gig” quest IDs and push to `completedGigs`.
      - Discovered locations at ~611: enumerate mappins/fast travel points via game APIs and collect IDs.
      - Vehicle states at ~628: query owned/spawned vehicles from VehicleSystem, capture pos/rot/condition/ownership.
      - World events at ~638: collect active events (NCPD, random encounters) from session state/event system.
    - Save restriction logic ~537–556: implement checks against combat/cutscene states and restricted areas beyond PSM high-level.
  - Persistence: wire `SaveGameManager.cpp/hpp` to actually serialize `SaveGameData` to an on-disk format compatible with CP2077 (or a parallel coop save under `SavedGames/Coop/`), then ensure load path reconstructs `PlayerSaveState` and `WorldSaveState` faithfully.

- Voice System — PARTIAL (VoiceManager.cpp, CoopVoice.reds)
  - `src/voice/VoiceManager.cpp` TODOs:
    - Initialize Opus encoder/decoder in `InitializeCodecs()` (~328–339) and dispose in `CleanupCodecs()`.
    - Implement `EncodeVoiceData()` and `DecodeVoiceData()` (~565, ~577) via libopus; decide frame size to match `CoopVoice::kOpusFrameBytes`.
    - Basic noise suppression/VAD: fill `ApplyNoiseReduction()` (~555) and integrate VAD checks in `ProcessingThreadMain` (~411ff) to gate `m_isTransmitting`.
    - Device stubs: complete audio device init/cleanup (capture/playback) at TODOs (~307, ~347, ~361, ~369, ~383).
  - `src/audio/CoopVoice.reds` is mostly wrapper; keep signature parity with C++ and feed/consume encoded frames accordingly.

- Authentication & Security — PARTIAL
  - Passwords: UI and `ConnectionManager.reds` anticipate passwords (`ConnectWithPassword()`), but `Net_ConnectToServer` currently takes `(host, port)` only. Add an overload or extend the existing native to include `password`, and propagate it into the handshake so server validates it.
  - Bans: Server has persistent ban storage (`src/server/DedicatedServer.cpp` loads/saves `banned_ips.txt`); also `AdminController.cpp` maintains `g_banList` and JSON path helpers. Connect these to `Net_IsPlayerBanned()` and `Net_BanPlayer()` in the net layer so checks occur at connect time (pre-accept).
  - Packet rate limiting/replay: Nonce-window logic exists in receive path; extend it across all encrypted message types and add rate limiting to heavy endpoints.
  - Input validation: ensure all admin commands parse and validate (`DedicatedServer.cpp` admin command parsing around lines ~200–615); sanitize file paths.

- Quest Synchronization — PARTIAL
  - `src/net/PhaseBundle.cpp` ApplyPhaseBundle now dispatches via `RED4EXT_EXECUTE("QuestSync", "ApplyFullSync", ...)`.
  - `src/server/QuestWatchdog.cpp` is robust (stage aggregation, branch votes, ending vote, cine triggers) but still relies on correct broadcast wiring (ensure `Net_BroadcastQuestStage(P2P)` and resync request/response are exercised on join and divergence detection).

- Memory Leak Prevention — UNVERIFIED
  - Targets for audit: network packet assembly paths (`Net.cpp` stack/heap usage), voice buffers (`VoiceManager` maps and per-player buffers), asset streaming queues (`AssetSync`), HTTP async storage (`HttpClient`). Add ownership comments and RAII wrappers where applicable; ensure no repeated `new` without `delete` on reconnects.

- Exception/Error Handling — PARTIAL
  - Standardize on `Logger` (exists in `src/core/Logger.hpp/.cpp`) and structured levels; replace `std::cout/cerr` scattered across `Net.cpp`, controllers, and vehicle/audio code.
  - Wrap risky operations: decompression (`ZSTD_*` in `PhaseBundle.cpp`/`WorldMarkers.cpp`), JSON/string parsing in server code (`ApartmentController`, `Journal`, etc.).

- Input/Packet Validation — PARTIAL
  - Guards present in `Net.cpp` for packet length before header read (checks at ~351–374). Apply similar size/bounds checks to all payload deserializations and to all `memcpy` calls (`Net.cpp`, `WorldMarkers.cpp`, `Snapshot.hpp`). Clamp counts to protocol max (example in `WorldMarkers.cpp` already clamps to 16-bit count).

- Inventory Synchronization Backend — PARTIAL
  - Inventory snapshot call aligned: `InventorySync.reds` now flattens `PlayerInventorySnap` to `(peerId, array<Uint64> itemIds, money)` and calls the existing native.
  - Placeholders in `InventorySync.reds`:
    - `GetPlayerPeerId()` returns constant (line ~137) — replace with native mapping to real peer ID.
    - Database natives (`InventoryDB_*`) are registered in `CoopExports.cpp` but may be placeholders — back them with a minimal embedded store or stub them out cleanly and gate their use.

- Vehicle Physics Integration — PARTIAL
  - `src/server/VehicleController.cpp` implements validation, damage capping, interpolation buffer, and passenger sync. Extend with:
    - Better interpolation under high latency: slerp rotation, Hermite for position; buffer N>3 samples.
    - Multi-passenger authority conflicts: resolve when two requests contend for the same seat.
    - Validate client-reported collisions against authoritative physics (seeded replay or simpler AABB checks).

- Performance Monitoring — PARTIAL
  - Add periodic `NetStats` sampling and expose via UI (`src/ui/MultiplayerUI.*` has `UpdateNetworkStats`). Add hooks to log spikes. Consider a simple ring buffer of frame-time/network-time and a JSON exporter for manual inspection.

- Asset Streaming — PARTIAL
  - `src/core/AssetSync.*` contains bandwidth management. Add prioritization tables instead of hardcoded priorities, and robust cache invalidation (content hash-based) for updates.

- Network Security — PARTIAL
  - Ensure encryption coverage for all sensitive packets; nonce replay window already implemented in receive path. Expand with HMAC for certain administrative messages.

- Database Integration — UNIMPLEMENTED
  - Start with flat-file/JSON for stats and ban persistence (already present for bans in some form). If moving to SQLite/Postgres later, define a minimal repository interface to avoid hard coupling.

- Content/Activity Systems — UNIMPLEMENTED/ROADMAP
  - Keep roadmap in design docs; don’t block core stability on these. When ready, introduce a plugin-driven activity registration to avoid growing core complexity.

- UI/UX Polish
  - Accessibility/localization: add string tables and high-contrast mode toggles; audit focus order and keyboard navigation in `ModernServerBrowser.reds` and `ServerHostDialog.reds`.

- Testing Expansion
  - `src/test/NetworkTest.cpp` and `test_network.cpp` exist. Add tests for: server password, ban list enforcement, quest resync full cycle, inventory send/receive roundtrip, and voice buffer encode/decode (once Opus is wired).

- Network Implementation Gaps — From docs vs code
  - `Net_StopServer()` and `Net_SetServerPassword()` are declared in `Net.hpp` but no definitions found in `Net.cpp`. Implement:
    - `Net_StopServer()`: broadcast disconnect to all peers, destroy `g_Host` under `g_NetMutex`, clear `g_Peers` safely.
    - `Net_SetServerPassword()`: persist to server config and validate in connect handshake; expose native for UI.
  - `Net_GetServerInfo()` referenced by tests is missing from headers — either add it (struct with name, playerCount, maxPlayers, password flag, mode) or update tests.
  - Verify concrete definitions for `Net_BroadcastAvatarSpawn()` and `Net_BroadcastPlayerUpdate()` are present in `Net.cpp` (declared in `Net.hpp`); add if missing.

- InventorySync.reds Placeholders (additional)
  - Replace local constants with session-aware natives (peer ID, money/version retrieval if still needed by logic).
  - Ensure native marshalling supports REDscript `struct` parameters for `PlayerInventorySnap` (see `CoopExports.cpp` placeholder handlers around ~1097ff).

- Phase Plan Coverage
  - P4-1 (Quest stage broadcast): `QuestSync.reds` implements `SendQuestStageMsg()`; ensure hook to `QuestSystem::AdvanceStage` is either actively wired or covered by existing `OnAdvanceStage()` usage.
  - P2-3 (Interpolation buffer): integrate buffer from `VehicleController` approach for player snapshots as well, or rely on `SnapshotInterpolator.reds` if present.
  - P8-1 (Thread-safety pass): this todo enumerates exact net-layer fixes; close this after `g_NetMutex` protection rollout and basic TSan pass.


Deep Review Findings (file-by-file)

- Core (cp2077-coop/src/core)
  - CoopExports.cpp
    - Numerous TODO placeholders for actual network sends (≈1950–2210): replace logs with real Net_Send/Net_Broadcast and proper struct packing.
    - Player/state helpers (≈1697–1926): implement object position/health/money getters/setters via game systems instead of TODO comments.
    - Inventory send handlers (~1097–1112): marshal PlayerInventorySnap from REDscript; remove placeholders.
    - Ensure Net_Init() and Net_Shutdown() calls (present around ~116, ~1629, ~1670) remain guarded against double calls.
  - AssetSync.cpp/hpp
    - Placeholder note about network integration (~617): wire bandwidth manager and streaming push/pull; replace hardcoded priorities; implement content-hash cache invalidation.
    - Review bandwidth history cleanup and reset logic (~628–639) for precision and thread-safety if used cross-thread.
  - WindowsCompat.cpp
    - glibc runtime check stub around ~180; verify platform guards.
  - GameClock.cpp
    - Prior analysis flagged thread-safety concerns. Validate access to tick fields; ensure monotonic tick calculations and non-zero GetTickMs (PhaseGC uses it).
  - SaveFork.cpp / SaveMigration.cpp
    - Prior analysis: buffer/format/path handling risks. Audit all string parsing and buffer writes; add bounds checks and sanitize paths.
  - SpatialGrid.cpp/hpp
    - Ensure namespace structure is consistent; earlier flagged duplicate namespace risk. Confirm inline helpers are defined/used; unit test proximity queries.
  - TaskGraph.cpp/hpp
    - Prior analysis: possible deadlock in resize. Audit all locks and reallocation patterns.
  - Logger.hpp/.cpp
    - Present and thread-safe; unify log usage across modules; remove direct cout/cerr callsites.

- Networking (cp2077-coop/src/net)
  - Net.hpp/.cpp
    - Missing definitions: Net_StopServer(), Net_SetServerPassword(), Net_GetServerInfo(); add and update tests.
    - Thread-safety: guard all g_Host/g_Peers/g_nextPeerId read/write with g_NetMutex; add running gate for shutdown.
    - Handshake: add password to Net_ConnectToServer signature and handshake; enforce Net_IsPlayerBanned at connect (currently TODO ~1955).
    - Receive path: maintain header/payload size checks; replicate for all memcpy payloads.
  - Connection.cpp
    - Controller proxies rely on RED4EXT_EXECUTE; ensure all packet handlers validate inputs (e.g., Perk/Skill/Elevator/Trade flows) and add error logging on invalid state.
    - Bundle cache protected via shared_mutex — good. Review any unguarded access sites.
  - PhaseBundle.cpp
    - TODO: replace commented RTTI with working RED4EXT_EXECUTE or updated RTTI call to QuestSync.ApplyFullSync.
    - ZSTD size guards present; keep sanity caps.
  - WorldMarkers.cpp
    - Correct 16-bit clamp and size validation before memcpy; model for similar code.
  - InterestGrid / NatClient / NatTraversal
    - Ensure thread-safety accessing any globals; implement TURN credential storage hardening and robust timeout/resource cleanup.
  - Snapshot.hpp / SnapshotWriter.cpp
    - Many memcpy uses; ensure bounds validated against buffer sizes; remove legacy .reds includes (prior fixes already applied in this codebase).

- Server (cp2077-coop/src/server)
  - DedicatedServer.cpp
    - TODOs: implement Net_DisconnectPlayer/Net_GetPlayerInfo, Net_BroadcastChatMessage, logger level setting, Net_DisconnectAllClients, proper player search (lines ≈285, 297, 310, 446, 534, 572). Use Net_GetConnections() and Connection state checks.
    - Password config read/write present; ensure handshake uses it.
  - DedicatedServer.hpp
    - Stores password and banned IP set; ensure APIs enforce these consistently.
  - DedicatedMain.cpp
    - Keep init/shutdown order; previously fixed variable order — recheck includes and lifecycle.
  - AdminController.cpp
    - JSON ban file parsing is manual; add robust parser or rework to small well-tested routines; sanitize inputs; salt usage helper present.
  - InfoServer.cpp / WebDash.cpp
    - Replace fixed payloads with live server data; sanitize HTTP inputs; platform-guard socket code.
  - QuestWatchdog.cpp
    - Solid voting/divergence handling; ensure broadcasts and resync are exercised and phase maps cleared safely.
  - ElevatorController.cpp
    - Arrival ack/retry logic is present with mutex; confirm paused state logic when peers leave mid-sequence.
  - PoliceDispatch.cpp
    - Timers converted to 64-bit; confirm spawn seeds and cadence scale with peers; cap wave indices as needed.
  - PhaseGC.cpp
    - TickMs zero handled; ensure cleanup never dereferences freed resources; clear dependent systems in safe order.
  - TrafficController / NpcController / StatusController / PerkController / SkillController / VendorController / TradeController
    - Validate all parameter bounds and object existence; ensure thread-safe access to shared structures; confirm trade flows maintain invariants.
  - ApartmentController.cpp / Journal.cpp / SectorLODController.cpp / CarryController.cpp / Heartbeat.cpp
    - Inputs: sanitize parsing; avoid per-call IO on hot paths (Journal); implement Windows mem info alternative (SectorLODController); avoid broadcasting zeroed vectors (CarryController); validate pointer usage (Heartbeat).
  - VehicleController.cpp
    - Present: damage validation, interpolation buffer, seat assignment. Improve: slerp rotation, Hermite interpolation, seat contention resolution, server-authoritative collision validation.

- Voice (cp2077-coop/src/voice)
  - VoiceManager.cpp
    - Fill audio device init/cleanup, Opus init/cleanup, encode/decode, VAD/noise reduction; mutex-protect shared maps/buffers; bound growth.
  - VoiceDecoder.cpp
    - Prior analysis: thread-safety and sequence wraparound; implement; add unit guard for packet ordering.

- Physics (cp2077-coop/src/physics)
  - VehiclePhysics.cpp / CarPhysics.cpp
    - Prior analysis: undefined calls and logic bugs; align with deterministic step; add compile guards and complete stubs.
  - LagComp.cpp
    - Bounds checking for RTT and rewind windows.

- REDscripts (cp2077-coop/src/runtime, src/gui, src/connection)
  - QuestSync.reds
    - Confirm hook targets exist in current build (~199); ensure OnAdvanceStage is wired or a polling fallback is active.
  - SaveGameSync.reds
    - Implement time/weather/gig/location/vehicle/world events getters; enforce save restrictions; integrate with SaveGameManager.
  - InventorySync.reds
    - Unify native signature for Net_SendInventorySnapshot; replace placeholders (GetPlayerPeerId, inventory extraction/manipulation, world item removal). Ensure DB natives aren’t no-ops at runtime.
  - PlayerSync.reds
    - Replace placeholder spawn with real proxy; fix deltaTime calculation in Update() (currently subtracts identical sim times); smooth interpolation.
  - CutsceneSync.reds / ElevatorSync.reds / PhotoModeBlock.reds / PopupGuard.reds
    - Verify class/method hooks; add optional/fallback paths; avoid hard failures if unavailable.
  - GrenadeSync.reds / GameEventHooks.reds
    - Implement safe object lookup and sync; add UI widget creation where TODO indicates.
  - GUI: ModernServerBrowser.reds / ServerHostDialog.reds / MainMenuInjection.reds / ChatOverlay.reds / DMScoreboard.reds
    - Verify Codeware optional dependency; ensure event handlers unregister on cleanup; audit accessibility and keyboard navigation; localization string tables.
  - ConnectionManager.reds
    - Calls Net_ConnectToServer with password param; add native; ensure dialog flows clear and reconnect logic handles kicked/banned states.

- Tests (cp2077-coop/src/test)
  - NetworkTest.cpp / test_network.cpp
    - Update tests after implementing Net_StopServer/SetServerPassword/GetServerInfo; add coverage for password auth, ban enforcement, quest resync, inventory snapshot, and voice encode/decode.
