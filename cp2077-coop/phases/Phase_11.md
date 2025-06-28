# Phase 11

```yaml
###############################################################################
# ███  ENTITY INTEREST-MANAGEMENT & ADAPTIVE TICK-RATE  (IMT-1 … IMT-4)     █
###############################################################################

- ticket_id: "IMT-1"
  summary: "Spatial grid & subscription table"
  context_files: []
  spec: >
    Create a 2-D spatial grid to bin dynamic entities for interest checks:
      • `src/runtime/SpatialGrid.reds` (cell size 32 m)
          func Add(id: Uint32, pos: Vector3);
          func Move(id: Uint32, newPos: Vector3);
          func Remove(id: Uint32);
          func Query(pos: Vector3, radius: Float, out ids: array<Uint32>);
      • Dedicated server owns a `SpatialGrid` instance and populates it
        with AvatarProxy, NpcProxy, VehicleProxy.
      • Per-client `interestSet : Set<Uint32>` stored in Connection.
  hints:
    - grid key = `int2(floor(x/size), floor(y/size))` hashed to Uint64.

- ticket_id: "IMT-2"
  summary: "Interest diff & delta send"
  context_files:
    - path: src/net/SnapshotWriter.hpp
      excerpt: |
        // SnapshotWriter already serialises entities
  spec: >
    Broadcast only relevant entities:
      • Every tick, server builds `newInterest = SpatialGrid.Query(avatarPos, 120 m)`.
      • Send `InterestAdd {id}` for entities newly in range.
      • Send `InterestRemove {id}` for entities that left range.
      • SnapshotWriter serialises *only* ids in `interestSet`.
  hints:
    - Track diff with `std::unordered_set` operations.

- ticket_id: "IMT-3"
  summary: "Adaptive tick-rate algorithm"
  context_files: []
  spec: >
    Scale simulation frequency vs. load:
      • In `GameClock.cpp`, add `currentTickMs` variable.
      • Compute moving average of `server_frame_time_ms` over 1 s window.
      • If avg > 25 ms ❯ lower tick-rate to 40 ms (25 Hz).
        If avg < 12 ms for 2 s ❯ raise tick to 25 ms (40 Hz).
      • Broadcast `TickRateChange` packet to clients.
  hints:
    - Clamp between 20–50 ms (50–20 Hz).

- ticket_id: "IMT-4"
  summary: "Client interpolation re-tune"
  context_files:
    - path: src/runtime/SnapshotInterpolator.reds
      excerpt: |
        const defaultInterpMs = 100
  spec: >
    React to tick-rate changes:
      • Add handler for `TickRateChange` packet that sets
        `interpDelayMs = max(2 * newTickMs, 80)`.
      • Update buffer logic to respect new delay.
  hints:
    - Keep delay ≥80 ms to avoid choppiness.



###############################################################################
# ███  SPECTATOR & ADMIN  TOOLS  (SA-1 … SA-3)                               █
###############################################################################

- ticket_id: "SA-1"
  summary: "Spectator camera mode"
  context_files: []
  spec: >
    Implement free-fly camera:
      • Add `Spectate` to GameModeManager enum.
      • `src/runtime/SpectatorCam.reds`:
          func EnterSpectate(peerId);
          func UpdateInput(dt);
      • `/spectate <peerId>` console cmd requests server; server replies
        with `SpectateGranted`.
  hints:
    - Disable player collision; hide HUD.

- ticket_id: "SA-2"
  summary: "Kick / ban / mute commands"
  context_files: []
  spec: >
    Server-side admin RPCs:
      • `EMsg::AdminCmd {cmdType,u64 param}` where cmdType ∈ Kick,Ban,Mute.
      • `src/server/AdminController.cpp` parses console input:
          kick <peerId>
          ban  <peerId>
          mute <peerId> <secs>
      • Update Connection to respect mute flag for ChatMsg.
  hints:
    - Ban list persisted later via PP-tickets.

- ticket_id: "SA-3"
  summary: "Live server dashboard (HTML)"
  context_files: []
  spec: >
    Expose lightweight WebSocket dash (localhost:7788):
      • `src/server/WebDash.cpp` serves `/status` JSON with
        peer list, ping, location, mode.
      • HTML page polls every 2 s and renders table.
  hints:
    - Use tiny-http (header-only) for stub; actual UI static html.



###############################################################################
# ███  PERSISTENCE & PROGRESS  MERGE  (PP-1 … PP-3)                          █
###############################################################################

- ticket_id: "PP-1"
  summary: "Session-end JSON snapshot"
  context_files: []
  spec: >
    Write a full session record:
      • At host shutdown, call `SaveSessionState(sessionId)`:
          – Party members & XP
          – QuestStageMap {questName:stage}
          – Inventory array<ItemSnap>
      • File path: `SavedGames/Coop/<sessionId>.json`.
  hints:
    - Reuse ItemSnap struct and QuestSync.

- ticket_id: "PP-2"
  summary: "Merge wizard CLI"
  context_files: []
  spec: >
    Tool `coop_merge.exe`:
      • Args: `<session.json> <singleplayerSave.dat>`.
      • Performs three-way merge:
          – If quest stage higher in coop, use coop.
          – Add missing inventory items if no conflicts.
          – Combine XP, keep highest.
      • Outputs `merged.dat` and prints diff summary.
  hints:
    - Use rapidjson for JSON and stub single-save parser.

- ticket_id: "PP-3"
  summary: "Conflict UI prompt"
  context_files: []
  spec: >
    In game, when importing coop progress, show GUI:
      • `src/gui/MergePrompt.reds` lists conflicts (quests/items).
      • Player can AcceptAll or SkipEach.
      • Selected resolution written to merged file.
  hints:
    - Conflicts array passed from coop_merge via JSON.



###############################################################################
# ███  NAT  TRAVERSAL / RELAY  (NT-1 … NT-3)                                 █
###############################################################################

- ticket_id: "NT-1"
  summary: "STUN discovery"
  context_files: []
  spec: >
    Integrate `libjuice` (ICE) for NAT traversal:
      • `src/net/NatClient.cpp`:
          func StartNat();  // performs STUN, gets public IP:port
          signal OnCandidate(candStr);
      • CMake find-package Juice; add to link.
  hints:
    - Use Google STUN server `stun.l.google.com:19302` for test.

- ticket_id: "NT-2"
  summary: "Peer ICE handshake"
  context_files:
    - path: src/net/Net.cpp
      excerpt: |
        // ENet host already exists
  spec: >
    Wrap ENet channel in ICE transport:
      • When both peers exchange candidates via master-server, establish
        UDP tunnel then attach ENet host to connected socket fd.
      • Add `Nat_PerformHandshake(peerId)` helper.
  hints:
    - Document fallback to relay if ICE fails.

- ticket_id: "NT-3"
  summary: "TURN relay fallback"
  context_files: []
  spec: >
    Use public Coturn relay:
      • If ICE fails within 5 s, request relay creds from master via
        `https://coop-master/api/turnCred`.
      • Re-attempt connection using supplied TURN server.
      • Log bandwidth used in Connection stats.
  hints:
    - Placeholder REST call; hard-coded creds fine.



###############################################################################
# ███  CUTSCENE CAMERA & LIP-SYNC  (CC-1 … CC-3)                             █
###############################################################################

- ticket_id: "CC-1"
  summary: "Cinematic time-code broadcast"
  context_files: []
  spec: >
    Sync camera tracks:
      • Add `EMsg::CineStart {sceneId, startTimeMs}`.
      • Server sends at cut-scene begin; clients seek their camera to
        exact `startTimeMs` offset to avoid drift.
  hints:
    - Hook `gamevision::StartCinematic`.

- ticket_id: "CC-2"
  summary: "Lip-sync viseme track"
  context_files: []
  spec: >
    Viseme packet:
      • `EMsg::Viseme {npcId, visemeId, timeMs}` sent every 250 ms.
      • Client queues viseme events for local mouth anim playback.
  hints:
    - visemeId enum comment (AA,TH,FV etc.)

- ticket_id: "CC-3"
  summary: "Dialogue choice sync"
  context_files: []
  spec: >
    When dialogue wheel appears:
      • Client sends `DialogChoice {choiceIdx}` on selection.
      • Server validates, then broadcasts same to peers.
      • Peers auto-advance dialogue to same branch (no UI).
  hints:
    - Use quest system `ConversationStateMachine` call.



###############################################################################
# ███  VOICE CHAT  (VC-1 … VC-3)                                             █
###############################################################################

- ticket_id: "VC-1"
  summary: "Opus encoder integration"
  context_files: []
  spec: >
    Build voice pipeline:
      • Add third-party `libopus` (48 kHz mono).
      • `src/voice/VoiceEncoder.cpp`:
          bool StartCapture(deviceName);
          int  EncodeFrame(int16* pcm, uint8_t* outBuf);
      • Packets: `EMsg::Voice {seq, opusBytes}`.
  hints:
    - Use 20 ms frames (960 samples).

- ticket_id: "VC-2"
  summary: "Jitter buffer & decoder"
  context_files:
    - path: src/voice/VoiceEncoder.cpp
      excerpt: |
        // EncodeFrame done
  spec: >
    Client side:
      • `VoiceJitterBuffer` reorders by seq (50-packet window).
      • `VoiceDecoder.DecodeFrame()` outputs PCM to XAudio2/AL.
      • Drop if late >200 ms.
  hints:
    - Placeholder audio out: write to ring-buffer.

- ticket_id: "VC-3"
  summary: "Push-to-talk integration"
  context_files:
    - path: src/gui/CoopSettings.reds
      excerpt: |
        var pushToTalk : EKey
  spec: >
    Capture mic only on PTT:
      • Add key-down check in `ChatOverlay.Toggle()` or HUD tick.
      • Show small mic icon when transmitting.
      • Option in settings to enable VOX (voice-activation) later.
  hints:
    - Use existing HUD layer for mic icon overlay.
```