# Phase 15

```yaml
###############################################################################
# ███  PHASE HJ — HOT-JOIN  STATE PREFETCH  (HJ-1 … HJ-2)                    █
###############################################################################

- ticket_id: "HJ-1"
  summary: "World-marker snapshot schema"
  context_files: []
  spec: >
    Package minimap & activity markers for late joiners:
      • Add `EMsg::WorldMarkers` with payload:
          Uint16  blobBytes
          uint8_t zstdBlob[blobBytes]
      • New file `src/net/WorldMarkers.cpp`:
          std::vector<uint8_t> BuildMarkerBlob();
          void ApplyMarkerBlob(const uint8_t* buf,size_t len);
      • BuildMarkerBlob collects:
          - gig job locations  (QuestSystem.ListOpenActivities)
          - player pins        (MinimapSystem.GetCustomPins)
          - NCPD events within 500 m of any player
      • Compress via zstd level 1 (`third_party/zstd`).
  hints:
    - Keep blob ≤10 kB; skip large icon atlases.

- ticket_id: "HJ-2"
  summary: "Late-join handshake"
  context_files:
    - path: src/net/Connection.cpp
      excerpt: |
        // OnJoinAccept handler
  spec: >
    Deliver markers after core snapshot:
      • After SnapshotAck from late-join client, server sends WorldMarkers.
      • Client decodes and calls ApplyMarkerBlob before showing HUD.
      • Add log line “[HotJoin] markers ready”.
  hints:
    - Sequence: JoinAccept → Snapshot → SnapshotAck → WorldMarkers.



###############################################################################
# ███  PHASE QS2 — QUEST SOFT-LOCK WATCHDOG  (QW-1 … QW-2)                  █
###############################################################################

- ticket_id: "QW-1"
  summary: "Divergence detector timer"
  context_files:
    - path: src/runtime/QuestSync.reds
      excerpt: |
        // divergence log
  spec: >
    Timed stage parity check:
      • In QuestSync, maintain `stageMap : Dict<CName,Uint16>` per peer.
      • On every QuestStage packet, update map and call CheckParity():
          – If variance >1 stage for same quest across peers
            and duration >15 s, flag soft-lock.
      • Flag triggers QuestResyncRequest to diverging peers.
  hints:
    - Use TimerSystem; check every 3 s.

- ticket_id: "QW-2"
  summary: "Forced resync response"
  context_files: []
  spec: >
    Server replies to QuestResyncRequest:
      • Build `QuestFullSyncPkt` (questName,stage) array.
      • Diverging client applies and logs “[Watchdog] forced sync”.
      • Add counter to prevent spam (max 2 per 5 min).
  hints:
    - Use same packet when new peer joins (reuse code).



###############################################################################
# ███  PHASE SC — SAVE  COMPRESSION  (SC-1)                                  █
###############################################################################

- ticket_id: "SC-1"
  summary: "Zstd compression & backup rotation"
  context_files:
    - path: src/core/SaveFork.cpp
      excerpt: |
        // SaveSessionState writes raw JSON
  spec: >
    Shrink session JSON on disk:
      • Add `third_party/zstd` and CMake link.
      • Replace raw write with:
          buf = zstd_compress(jsonStr, level=3);
          write `<sessionId>.json.zst`.
      • Rotate backups: keep newest 5, delete older.
      • Update coop_merge to detect `.zst` and decompress.
  hints:
    - Add “Content-Encoding: zstd” note in header comments.



###############################################################################
# ███  PHASE HUD — STAT / LATENCY OVERLAY  (HU-1 … HU-2)                     █
###############################################################################

- ticket_id: "HU-1"
  summary: "Net stat collector"
  context_files: []
  spec: >
    Aggregate per-peer metrics:
      • In Connection, track:
          pingMs (updated on Pong), pktLoss %, voiceKBs, snapKBs.
      • New struct `NetStats { Uint32 ping; Float loss; Uint16 vKbps; Uint16 sKbps; };`
      • Every 2 s push to HUD via event bus.
  hints:
    - loss = lost/(sent+lost) rolling 8-s window.

- ticket_id: "HU-2"
  summary: "In-game HUD panel (F1)"
  context_files:
    - path: src/gui/StatHud.reds   # new file
      excerpt: |
        // TBD
  spec: >
    Draw table overlay:
      • Toggle with F1; uses inkCanvas top-left.
      • Columns: Peer, Ping, Loss %, Voice Kbps, Snap Kbps, TickRate.
      • Auto-colour ping: 0-80 ms green, 81-150 yellow, >150 red.
  hints:
    - Pull tickRate from GameClock.currentTickMs.



###############################################################################
# ███  PHASE SP — SPECTATOR & FREECAM  (SP-1 … SP-3)                         █
###############################################################################

- ticket_id: "SP-1"
  summary: "POV cycle & freecam toggle"
  context_files: []
  spec: >
    Controls:
      • Key “B” cycles target avatar (Next/Prev with mouse wheel).
      • Key “N” toggles freecam mode.
      • Freecam speed adjustable with QE keys.
  hints:
    - Store `currentTargetId` in SpectatorCam.

- ticket_id: "SP-2"
  summary: "Freecam bounds enforcement"
  context_files:
    - path: src/runtime/SpectatorCam.reds
      excerpt: |
        // UpdateInput()
  spec: >
    Prevent streaming hole:
      • Each frame, if distance from nearest avatar >1000 m,
        clamp position back to 1000 m boundary.
      • Show warning text “Out-of-range”.
  hints:
    - Use squared distance for perf.

- ticket_id: "SP-3"
  summary: "Smooth camera blend"
  context_files: []
  spec: >
    When switching POV:
      • Lerp position & rotation over 0.6 s using cubic ease-in-out.
      • Disable input during blend to avoid nausea.
  hints:
    - Start blend on SetTarget().



###############################################################################
# ███  PHASE GF — GRIEF / FRIENDLY-FIRE  (GF-1 … GF-2)                       █
###############################################################################

- ticket_id: "GF-1"
  summary: "CFG flag & damage filter"
  context_files: []
  spec: >
    Add server CFG var `friendly_fire` bool (default false).
      • Parse in coop_dedicated.ini.
      • DamageValidator: if sourcePeer == targetPeer OR same team
        and friendly_fire == false ⇒ clamp damage 0.
  hints:
    - team concept = all players vs. NPCs for now.

- ticket_id: "GF-2"
  summary: "Spawn-kill protection"
  context_files:
    - path: src/runtime/Respawn.reds
      excerpt: |
        // PerformRespawn()
  spec: >
    Grant 5-s invulnerability after respawn:
      • AvatarProxy.invulEndTick = nowTick + (5000/ tickMs).
      • DamageValidator bypass if currentTick < invulEndTick.
      • Flicker shader effect on protected player.
  hints:
    - Remove flicker on first shot fired.



###############################################################################
# ███  PHASE DD — DYNAMIC DIFFICULTY  (DD-1 … DD-2)                          █
###############################################################################

- ticket_id: "DD-1"
  summary: "Spawn scaling formula"
  context_files: []
  spec: >
    Enemy health & damage multipliers:
      • In NpcController.ServerTick(), read `playerCount`.
      • healthMult = 1 + 0.25*(playerCount-1) (cap 2.0).
      • dmgMult    = 1 + 0.15*(playerCount-1) (cap 1.6).
      • Apply when spawning combat NPCs.
  hints:
    - Store originalBaseHP in template.

- ticket_id: "DD-2"
  summary: "Dynamic AI reinforcements"
  context_files:
    - path: src/runtime/NpcController.reds
      excerpt: |
        // random walk AI
  spec: >
    If >2 players and alert level High:
      • Spawn additional wave of 2 NPCs after 30 s.
      • Wave count limited to 3 per encounter.
      • Broadcast NpcSpawn packets accordingly.
  hints:
    - alert level stub: detect any player in combat state.



###############################################################################
# ███  PHASE PM — PHOTO-MODE DISABLE  (PM-1)                                 █
###############################################################################

- ticket_id: "PM-1"
  summary: "Block or mirror photo-mode"
  context_files: []
  spec: >
    Hook PhotoMode start:
      • Detour `PhotoModeSystem::Enter()`.
      • If GameMode != Coop:
          call original.
        Else:
          Show CoopNotice "Photo Mode disabled in multiplayer".
          Return early (skip pause toggle).
      • Log attempt.
  hints:
    - Use PopupGuard.ReplaceWithCoopNotice.



###############################################################################
# ███  PHASE MB — MEMORY  BUDGET GUARD  (MB-1 … MB-2)                        █
###############################################################################

- ticket_id: "MB-1"
  summary: "Snapshot heap monitor"
  context_files: []
  spec: >
    Track rolling memory usage:
      • Every 60 s, call `MemCheck()` in DedicatedMain.
      • Use `malloc_stats()` or `std::pmr::get_default_resource()->memory_resource` size if available.
      • If >2 GiB allocated by snapshots:
          Flush snapshot baselines > 5 min old.
          Log warning.
  hints:
    - Fallback to `mallinfo` on glibc.

- ticket_id: "MB-2"
  summary: "Voice buffer purge"
  context_files:
    - path: src/voice/VoiceJitterBuffer.cpp
      excerpt: |
        // todo purging
  spec: >
    Limit voice backlog:
      • Keep queue length ≤120 packets (≈2.4 s).
      • If exceeded, drop oldest and warn once per minute.
      • Add statistic voiceDropPkts to HU-1 NetStats.
  hints:
    - Update Stat HUD colour red if dropPkts >5 %.
```