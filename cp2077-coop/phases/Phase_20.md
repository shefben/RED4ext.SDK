# Phase 20

```yaml
###############################################################################
# ███  PHASE CA — CROWD AMBIENT CHATTER & GESTURE AI (CA-1)                 █
###############################################################################

- ticket_id: "CA-1"
  summary: "Replicate crowd chatter & gesture sync"
  context_files: []
  spec: >
    • Hook `AmbientChatterSystem::StartConversation(npcA,npcB,lineId)`.
    • Server authoritative: chooses `seed32` and broadcasts
      `CrowdChatterStart {npcA,npcB,lineId,seed}`.
    • Clients feed seed into local `LipSyncRand` and `GestureRand`
      to pick the same small-talk animation & viseme sequence.
    • When conversation ends, server sends `CrowdChatterEnd {convId}`.
  hints:
    - `convId = FNV(npcA,npcB,startTick)`.



###############################################################################
# ███  PHASE HB — DYNAMIC HOLO-BILLBOARDS (HB-1)                             █
###############################################################################

- ticket_id: "HB-1"
  summary: "Synchronise advert rotation"
  context_files: []
  spec: >
    • On sector load, server sends `HoloSeed {sectorHash,seed64}`.
    • Client BillboardController seeds RNG → picks ad playlist deterministically.
    • When campaign rotates (every 120 s), server broadcasts
      `HoloNextAd {sectorHash,adId}` so late joiners stay aligned.
  hints:
    - adId maps to hologram `.ent` via `Ads.csv`.



###############################################################################
# ███  PHASE DH — DOOR BREACH MINIGAME (DH-1)                                █
###############################################################################

- ticket_id: "DH-1"
  summary: "Breach-open progress bar sync"
  context_files: []
  spec: >
    • When player begins `DoorHackComponent::StartBreach()`
      → send `DoorBreachStart {doorId,phaseId,seed}`.
    • Server authoritative progress ticks (1 s baseline, −10 % per perk).
    • Every 0.25 s server broadcasts `DoorBreachTick {doorId,percent}`.
    • On success: `DoorBreachSuccess`; all clients unlock door physics.
  hints:
    - Cancel on combat hit; send `DoorBreachAbort`.



###############################################################################
# ███  PHASE FXE — ENVIRONMENTAL WEATHER PARTICLES (FXE-1)                   █
###############################################################################

- ticket_id: "FXE-1"
  summary: "Deterministic weather particle seed"
  context_files: []
  spec: >
    • Extend WorldState packet: add `uint16 particleSeed`.
    • Server changes seed whenever weatherId changes.
    • Client passes seed to `ParticleSystem.SpawnWeatherFX()`
      so splash/ dust storms spawn in same pattern.
  hints:
    - Keep seed constant during weather to avoid re-emit.



###############################################################################
# ███  PHASE HG — HOLOCALL GROUP OVERLAY (HG-1)                              █
###############################################################################

- ticket_id: "HG-1"
  summary: "Multi-peer holocall sync"
  context_files: []
  spec: >
    • Packet `HolocallStart {fixerId,callId,peerIds[]}`.
    • Each peer shows small holo face overlay at screen-left.
    • Subtitles streamed from VO queue (`VOPlay`) with prefix of speaker.
    • On end → `HolocallEnd {callId}` removes overlays.
  hints:
    - Overlay ignores input freeze.



###############################################################################
# ███  PHASE HTB — HOLO-TABLE / BRAIN DANCE REVIEW (HT-1)                    █
###############################################################################

- ticket_id: "HT-1"
  summary: "Shared holo-table timeline scrub"
  context_files: []
  spec: >
    • When player opens table, server sends `HTableOpen {sceneId}`.
    • `HTableScrub {timestampMs}` packets broadcast on scrub wheel move.
    • All peers preview BD frame at same time; non-owner UI read-only.
  hints:
    - Clamp scrub spam to 10 Hz.



###############################################################################
# ███  PHASE QG — QUEST-SPECIFIC GADGETS (QG-1)                              █
###############################################################################

- ticket_id: "QG-1"
  summary: "Totalimmortal rail-gun & Nanowire hacking sync"
  context_files: []
  spec: >
    • Packet `QuestGadgetFire {questId,gadgetType,parameters}`.
      – gadgetType 1 = rail-gun: parameters = charge%.
      – gadgetType 2 = nanowire: parameters = {targetId}.
    • Server validates quest & stage; on OK, broadcasts same pkt.
    • Clients spawn rail recoil FX or lock target ragdoll.
  hints:
    - gadgetType enum in `QuestGadget.hpp`.



###############################################################################
# ███  PHASE IP — ITEM PICK-UP PHYSICS (IP-1)                                █
###############################################################################

- ticket_id: "IP-1"
  summary: "Physics grab / pick-up anim sync"
  context_files: []
  spec: >
    • On `PhysicsGrabComponent::Attach(itemId)`, server sends
      `ItemGrab {peerId,itemId}`.
    • Client plays pick-up animation, hides physics sim, attaches to hand.
    • On `Detach`, sends `ItemDrop {pos}` or `ItemStore`.
  hints:
    - If Blackwall chip, trigger unique HUD event id 0xBBCHIP.



###############################################################################
# ███  PHASE SB — SUBWAY / NCART TRANSIT (SB-1 … SB-2)                       █
###############################################################################

- ticket_id: "SB-1"
  summary: "Metro boarding & car instance"
  context_files: []
  spec: >
    • `MetroBoard {peerId,lineId,carIdx}`; server assigns `phaseId=peerId` car.
    • Client loads interior scene “metro_car_a”.
    • Seat network handled like vehicle seats but `phaseId=peerId`.
  hints:
    - lineId string “green”, “red”.

- ticket_id: "SB-2"
  summary: "Metro exit & teleport"
  context_files:
    - path: src/runtime/TransitSystem.reds
      excerpt: |
        // TODO stub
  spec: >
    • At target station, server sends `MetroArrive {peerId,stationId}`.
    • Player choice of exit door teleports back to world sector hash.
  hints:
    - Use SectorReady handshake.



###############################################################################
# ███  PHASE RS — RADIO STATION SYNC (RS-1)                                  █
###############################################################################

- ticket_id: "RS-1"
  summary: "Vehicle radio station & timestamp"
  context_files: []
  spec: >
    • When player toggles radio, server sends
      `RadioChange {vehId,stationId,offsetSec}`.
    • All passengers & spectating peers set Wwise switch to stationId
      & seek to offsetSec.
  hints:
    - offsetSec = (GameClock.now / 1000) % songLength.



###############################################################################
# ███  PHASE SF — SECURITY CAMERA HIJACK (SF-1)                              █
###############################################################################

- ticket_id: "SF-1"
  summary: "Remote CCTV feed sharing"
  context_files: []
  spec: >
    • Netrunner quickhack “Camera Control” sends `CamHijack {camId,peerId}`.
    • While active, server streams `CamFrameStart {camId}` every 0.5 s
      (no JPEG—just trigger clients to render from that cam POV).
    • Other peers can press key to spectate cam (read-only).
  hints:
    - Stop stream on line-of-sight break.



###############################################################################
# ███  PHASE PC — PICK-UP & CARRY (PHYS OBJECT) (PC-1)                       █
###############################################################################

- ticket_id: "PC-1"
  summary: "Carry ragdoll / crate physics sync"
  context_files: []
  spec: >
    • On `CarryBegin {carrierId,entityId}`, server sets entity kinematic,
      updates transform every 100 ms via `CarrySnap`.
    • On drop, `CarryEnd {pos,vel}` restores physics.
  hints:
    - Max carry weight check on server.



###############################################################################
# ███  PHASE GR — GRENADE TRAJECTORY (GR-1)                                  █
###############################################################################

- ticket_id: "GR-1"
  summary: "Cook timer & spline sync"
  context_files: []
  spec: >
    • On pin pull, client sends `GrenadePrime {entityId,startTick}`.
    • Every simulation step, server integrates parabola; sends
      `GrenadeSnap {entityId,pos,vel}` every 50 ms.
    • Detonation at synced tick. 50 ms rewind for latency like VC-1.
  hints:
    - Cook time clamp 5 s.



###############################################################################
# ███  PHASE RC — SMART WEAPON KILLCAM (RC-1)                                █
###############################################################################

- ticket_id: "RC-1"
  summary: "Projectile cam sync"
  context_files: []
  spec: >
    • When smart gun killcam triggers, server sends `SmartCamStart {projId}`.
    • Camera spline computed server-side; clients follow same path.
    • On impact, `SmartCamEnd` returns control.
  hints:
    - Disable if LowBWMode.



###############################################################################
# ███  PHASE CD — CROWD DENSITY OVERRIDE (CD-1)                              █
###############################################################################

- ticket_id: "CD-1"
  summary: "Force density slider to ‘High’ for clients"
  context_files: []
  spec: >
    • When peer connects, server sends `CrowdCfg {density=High}`.
    • Client CrowdSettings overrides user INI while connected.
    • Restore original on disconnect.
  hints:
    - Show toast “Density locked for multiplayer stability”.



###############################################################################
# ███  PHASE RB — REFLEX BOOSTER BULLET-TIME (RB-1)                          █
###############################################################################

- ticket_id: "RB-1"
  summary: "Kill-cam slow-mo finisher sync"
  context_files: []
  spec: >
    • On perk trigger, server broadcasts `SlowMoFinisher {peerId,targetId,durMs}`.
    • All clients lerp global TimeScale to 0.3 for durMs, then restore.
    • Victim ragdoll driven in slow-mo on all machines.
  hints:
    - Clamp cumulative slow-mo stack.



###############################################################################
# ███  PHASE AM — ARCADE & BAR MINI-GAMES (AM-1)                             █
###############################################################################

- ticket_id: "AM-1"
  summary: "Roach Race high-score & input sync"
  context_files: []
  spec: >
    • Start: `ArcadeStart {cabId,peerId,seed}`.
    • Inputs: `ArcadeInput {frame,buttonMask}` deterministic like classic net-code.
    • Server tracks score; on GameOver broadcast `ArcadeScore {peerId,score}`.
    • High-score board stored per cab globally in SaveFork.
  hints:
    - seed = GameClock.tick.



###############################################################################
# ███  PHASE SA — SERVER AUTOSCALE / WORKER THREADS (SA-1 … SA-2)            █
###############################################################################

- ticket_id: "SA-1"
  summary: "Task graph & worker pool"
  context_files: []
  spec: >
    • Introduce `TaskGraph` with thread-pool (std::jthread) size = HW cores-1.
    • Offload NPC AI ticks, snapshot diff build, physics step to tasks.
    • Main thread only network I/O + game loop orchestration.
  hints:
    - Use lock-free queues (ThreadSafeQueue).

- ticket_id: "SA-2"
  summary: "Dynamic worker count autoscale"
  context_files: []
  spec: >
    • Every 5 s measure `avgFrameMs`.  
      – If >30 ms and `workers<max`, add worker.  
      – If <15 ms for 30 s and `workers>1`, remove one.
    • Log “Autoscale workers=N”.
  hints:
    - Protect resize with mutex.
```