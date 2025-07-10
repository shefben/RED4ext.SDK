# PHASE_PLAN.md  
*(Ticket queue for the nine-agent ChatGPT o3 swarm – each item is sized for one ≤ 20-minute pass.)*

> **How to use:**  
> 1. The **Conductor** pops the **next ticket** (top → bottom).  
> 2. Feeds it to **PromptSmith**, then the appropriate specialist.  
> 3. When a ticket finishes (`response_status: DONE`), move to the next.  
> 4. If it fails, re-queue the same ticket with the failure report attached.  

---

## 📦 Phase 0 — Bootstrapping

| ID       | Ticket (20 min max) | Deliverables / Acceptance Hint |
|----------|---------------------|--------------------------------|
| **P0-1** | **Blank mod skeleton** – create RED4ext folder layout, `CMakeLists.txt`, `manifest.json`, empty `.reds` entry point. | Repo builds an empty `.archive` & prints “loaded” on RED4ext console (no game logic yet). |
| **P0-2** | **Hot-reload harness** – add `REDScripts` stub that logs on reload.                                                  | Running `ReloadAllMods()` produces console line twice in a row.                           |
| **P0-3** | **Networking stub lib** – vendor ENet headers + minimal wrapper `Net_Init/Net_Shutdown()`.                           | `src/net/Net.cpp` & `Net.hpp` compile without undefined refs.                             |
| **P0-4** | **Headless server scaffold** – CLI target `coop_dedicated.exe` that links Net stub and prints “Dedicated up”.        | Executable starts and exits cleanly with `--help`.                                        |

---

## 🧩 Phase 1 — Core Session Pipeline

| ID  | Ticket | Deliverables / Acceptance Hint |
|-----|--------|--------------------------------|
| **P1-1** | **Packet enums & header** – define `EMsg` and 4-byte `{u16 type,u16 size}` helpers. | `src/net/Packets.hpp` exists; helper tests in `tests/static/packet_header.json` pass lint. |
| **P1-2** | **Connection states** – handshake → lobby → in-game; timeouts + ping. | State diagram comments + minimal switch in `Connection.cpp`. |
| **P1-3** | **Authoritative host flag** – hard-code `gIsDedicatedAuthority = true`. | Constant visible in `NetConfig.hpp`. |
| **P1-4** | **Fixed tick-lock** – expose `GameClock::GetCurrentTick()` at 32 ms. | Static test shows monotonic tick increments. |
| **P1-5** | **RNG seed sync** – server sends `SeedPacket`; client sets `std::mt19937`. | Static test: same seed yields equal `rand()` outputs in doc comment. |
| **P1-6** | **Delta-snapshot schema** – create `SnapshotWriter/Reader` skeleton with dirty-bit spec. | Spec commented; JSON sample in `tests/static/snapshot_schema.json`. |

---

## 🏃 Phase 2 — Player Replication Basics

| ID  | Ticket | Deliverables / Acceptance Hint |
|-----|--------|--------------------------------|
| **P2-1** | **Spawn / despawn RPCs** – net spawn proxy avatars; despawn on leave. | `AvatarProxy.reds` added; doc example shows T-pose spawn. |
| **P2-2** | **Transform sync** – serialize `pos,rot,vel` at 20 Hz with delta. | Struct `TransformSnap` in `Snapshot.hpp`. |
| **P2-3** | **Interpolation buffer** – 100 ms buffer + Hermite sample stub. | `SnapshotInterpolator.reds` returns smoothed vec3 (static math check). |
| **P2-4** | **Client prediction & reconciliation** – local authoritative move w/ correction hook. | Commented pseudo-math in `Prediction.reds`. |
| **P2-5** | **Health / armor model** – add to snapshot; floating bar prefab. | `HealthBar.reds` UI stub in `/src/gui/`. |
| **P2-6** | **Ragdoll on disconnect** – swap proxy to ragdoll + killfeed entry. | `OnPeerLeft()` sets `bRagdoll = true`. |

---

## 🖥️ Phase 3 — UI & Lobby Flow

| ID  | Ticket | Deliverables / Acceptance Hint |
|-----|--------|--------------------------------|
| **P3-1** | **Main-menu button** – inject “CO-OP” into title screen. | Button opens placeholder panel. |
| **P3-2** | **Server-browser panel** – scroll list fed by `/list` JSON; click selects. | `ServerList.reds` with sample JSON in `tests/static/serverlist.json`. |
| **P3-3** | **Join / Host buttons** – host spins hidden dedicated process; join connects. | Successful local loopback connect demonstrated in log comment. |
| **P3-4** | **In-game chat overlay** – toggle on `Enter`, reliable `ChatMsg` packet. | Chat log array length capped at 50 lines. |

---

## 🌍 Phase 4 — World & Quest Sync

| ID  | Ticket | Deliverables / Acceptance Hint |
|-----|--------|--------------------------------|
| **P4-1** | **Quest stage broadcast** – hook `QuestSystem::AdvanceStage`. | `QuestSync.reds` contains `SendQuestStageMsg()`. |
| **P4-2** | **Trigger / scene sync** – cutscene start/stop mirrored. | Comment: “All players enter braindance together”. |
| **P4-3** | **Inventory authority** – owner field for dropped items. | `ownerId` added to `ItemSnap` struct. |
| **P4-4** | **Net-safe pop-ups** – replace single-player sleeps, braindance. | `PopupGuard.reds` disables freeze. |
| **P4-5** | **Savegame forking** – co-op saves under `SavedGames/Coop/`. | Path constant in `SaveFork.cpp`. |

---

## 🔫 Phase 5 — Combat Fidelity

| ID  | Ticket | Deliverables / Acceptance Hint |
|-----|--------|--------------------------------|
| **P5-1** | **Hitscan lag compensation** – rewind N ms on server confirm. | `LagComp.hpp` math comment includes formula. |
| **P5-2** | **Melee rollback buffer** – 250 ms input buffer. | `Rollback.cpp` stub passes static time math test. |
| **P5-3** | **Quickhack sync** – replicate target & cooldown. | `QuickhackSync.reds` added. |
| **P5-4** | **Armor dmg calc server-side** – validate against hacks. | `DamageValidator.cpp` asserts bounds. |
| **P5-5** | **Shared NCPD heat** – sync stars & spawn waves. | `HeatSync.reds` includes enum. |
| **P5-6** | **World state sync** – time-of-day & weather. | `WeatherSync.reds` broadcasts every 30 s. |

---

## 🚗 Phase 6 — Vehicles & Mounts

| ID  | Ticket | Deliverables / Acceptance Hint |
|-----|--------|--------------------------------|
| **P6-1** | **Vehicle spawn replication** – server decides ID & transform. | `VehicleSnap` struct present. |
| **P6-2** | **Seat assignment RPC** – `EnterVehicleMsg`. | Seat index comment range 0-3. |
| **P6-3** | **Deterministic car physics** – authoritative server calculated. | `CarPhysics.cpp` includes fixed-step notes. |
| **P6-4** | **Collision damage sync** – `VehicleHitMsg`. | Sample JSON fixture in `tests/static/vehicle_hit.json`. |

---

## 🏅 Phase 7 — Simple Deathmatch Mode

| ID  | Ticket | Deliverables / Acceptance Hint |
|-----|--------|--------------------------------|
| **P7-1** | **DM ruleset toggle** – `/gamemode dm` RPC resets map. | `GameModeManager.reds` toggles enum. |
| **P7-2** | **Frag counter & HUD** – leaderboard panel (`Tab`). | `DMScoreboard.reds` UI stub. |
| **P7-3** | **Respawn system** – random point, 5 s timer. | `Respawn.reds` delay const. |
| **P7-4** | **Match timer & win** – 10-min round / 30 frags. | `MatchTimer.reds` comment describes flow. |
| **P7-5** | **Stat packet batching** – batch kill/death per tick. | `StatBatch.cpp` packs ≤1 KB/s. |

---

## 🧹 Phase 8 — Hardening & Polish

| ID  | Ticket | Deliverables / Acceptance Hint |
|-----|--------|--------------------------------|
| **P8-1** | **Thread-safety pass** – mutexes or lock-free ring buffers. | `ThreadSafeQueue.hpp` added. |
| **P8-2** | **Crash telemetry** – zip dump + net log and prompt to send. | `CrashHandler.cpp` path comment. |
| **P8-3** | **Version hash check** – compare CRCs on connect. | `VersionCheck.reds` rejects mismatches. |
| **P8-4** | **Master-server heartbeat** – `/register` ping every 30 s. | `Heartbeat.cpp` example JSON. |
| **P8-5** | **Settings panel** – tick-rate, interp delay, push-to-talk. | `CoopSettings.reds` saves `coop.ini`. |

---

### ✅ Completion Criteria

All tickets **P0-1 → P8-5** return `response_status: DONE` with `static_checks: lint-ok` or better.  
At that point the project forms a feature-complete, statically sound co-op mod ready for manual runtime validation.

*End of PHASE_PLAN.md*
