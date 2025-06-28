# Phase 16

```yaml
###############################################################################
# ███  PHASE PD — ADVANCED POLICE BEHAVIOUR & DISPATCH  (PD-1 … PD-4)       █
###############################################################################

- ticket_id: "PD-1"
  summary: "NCPD dispatch & wave escalation"
  context_files: []
  spec: >
    Server-authoritative police dispatch timer:
      • Add `src/runtime/PoliceDispatch.reds`
          var waveTimerMs  : Uint32 = 0
          var nextWaveIdx  : Uint8  = 0
          func Tick(dtMs);
      • When HeatSync.heatLevel increases:
          waveTimerMs = 0;   nextWaveIdx = 0
      • Tick(): every 30 s (­heat ≤2) or 15 s (­heat ≥3) spawn cruiser wave:
          Net_Broadcast(EMsg::NpcSpawnCruiser, payload {waveIdx,npcSeeds[4]})
      • Cruiser NPCs flagged `alwaysRelevant`.
  hints:
    - Use seed = FNV(peerCount, waveIdx) for deterministic patrol path.

- ticket_id: "PD-2"
  summary: "Pursuit AI state machine sync"
  context_files: []
  spec: >
    Sync police pursuit states:
      • Extend `NpcSnap` with `Uint8 aiState` (Idle,Search,Pursuit,Combat).
      • Police AI on server changes aiState and broadcasts
        `EMsg::NpcState {npcId, aiState}`.
      • Client NpcProxy switches behaviour tree accordingly.
  hints:
    - aiState enum comment in header.

- ticket_id: "PD-3"
  summary: "Scanner crime event replication"
  context_files: []
  spec: >
    Shared “blue crimes”:
      • Add `src/runtime/CrimeSpawner.reds` server-side.
      • When player crosses crime trigger, build seed and broadcast
        `CrimeEventSpawn {eventId, seed}` with list of spawned npcIds.
      • Clients instantiate the same crowd + evidence items.
  hints:
    - eventId = hash(triggerEntityName).

- ticket_id: "PD-4"
  summary: "Max-Tac arrival cinematic"
  context_files: []
  spec: >
    5-star response:
      • On heat==5 for >60 s, server sends `CineStart`, sceneId "maxtac_av".
      • Spawn Max-Tac squad via NpcSpawn pkt.
      • Broadcast `Viseme` packets during voice-over (reuse CC-2).
  hints:
    - Scene file path `cinematic/city/maxtac_drop.scene`.



###############################################################################
# ███  PHASE CW — CYBERWARE SYSTEMS (CW-1 … CW-3)                           █
###############################################################################

- ticket_id: "CW-1"
  summary: "EquipCyberware broadcast & validation"
  context_files: []
  spec: >
    • Add `EMsg::CyberEquip {peerId, slotId, itemSnap}`.
    • Server validates prerequisites (street cred, capacity).
    • On success, updates stats & Netrunner quickhack slots; client
      reloads arm/optic mesh.
  hints:
    - itemSnap struct already exists from IC-phase.

- ticket_id: "CW-2"
  summary: "Slow-mo cyberware sync"
  context_files: []
  spec: >
    • Broadcast `SlowMoStart {peerId, factor, duration}` when Sand-Dev
      activated.
    • Clients scale animation Δt by factor while flag set.
    • HeatSync tick unaffected (server uses real dt).
  hints:
    - factor 0.5–0.2 typical.

- ticket_id: "CW-3"
  summary: "Cyberware cooldown UI"
  context_files: []
  spec: >
    • Add `src/gui/CyberCooldownHud.reds` radial per equipped ware.
    • Cooldown values come from EquipCyberware packet + local timers.
    • Show grey-out overlay; pulse green when ready.
  hints:
    - Use ink radial progress.



###############################################################################
# ███  PHASE PK — PERK TREE & RESPEC  (PK-1 … PK-2)                          █
###############################################################################

- ticket_id: "PK-1"
  summary: "PerkSync & server authority"
  context_files: []
  spec: >
    • `EMsg::PerkUnlock {peerId, perkId, rank}`; client sends request,
      server validates and re-broadcasts.
    • Add `PerkMultiplier` look-ups in DamageValidator.healthMult.
  hints:
    - Put perks in hash map perkId→effects.

- ticket_id: "PK-2"
  summary: "Respec & XP merge"
  context_files: []
  spec: >
    • `/respec` UI sends `PerkRespecRequest`; server refunds perk points,
      re-calculates stats, sends `PerkRespecAck {newPoints}`.
    • SaveFork records perk tree under per-player block.
  hints:
    - Deduct €$ 100K cost on server.



###############################################################################
# ███  PHASE SE — STATUS EFFECT AURA  (SE-1)                                 █
###############################################################################

- ticket_id: "SE-1"
  summary: "DOT/CC effect replication"
  context_files: []
  spec: >
    • `EMsg::StatusApply {targetId,effectId,durMs,amp}`.
    • DamageValidator applies tick each 500 ms server-side; broadcasts
      `StatusTick {targetId,hpDelta}`.
    • Client spawns VFX `"burning_body.ent"` etc. via effectId.
  hints:
    - effectId enum: Burn=1,Shock=2,Poison=3,EMP=4.



###############################################################################
# ███  PHASE TF — DYNAMIC TRAFFIC & CIVILIAN CARS  (TF-1 … TF-2)             █
###############################################################################

- ticket_id: "TF-1"
  summary: "Ghost traffic seed stream"
  context_files: []
  spec: >
    • Every 10 s server sends `TrafficSeed {sectorHash, seed64}`.
    • Clients feed seed into local traffic sim (TrafficSystemRAND).
    • Only bounding boxes + horns; no deterministic collisions.
  hints:
    - Disable when distance <30 m to any player; switch to true physics.

- ticket_id: "TF-2"
  summary: "Traffic despawn consistency"
  context_files: []
  spec: >
    • Add `TrafficDespawn {vehId}` packet when host GC’s a vehicle.
    • Clients remove matching ghost to avoid orphan boxes.
  hints:
    - vehId = hash(meshPath,pos.



###############################################################################
# ███  PHASE PH — ENVIRONMENTAL DESTRUCTION  (PH-1 … PH-2)                  █
###############################################################################

- ticket_id: "PH-1"
  summary: "PropBreak broadcast"
  context_files: []
  spec: >
    • Hook `PhysicsProp::OnBreak()`.
    • Server sends `PropBreak {entityId,seed}`; clients spawn deterministic shard FX with seed.
  hints:
    - Use seed to choose shard impulse directions.

- ticket_id: "PH-2"
  summary: "Explosive barrel chain-reaction"
  context_files: []
  spec: >
    • When an explosive prop breaks, schedule radius query for other barrels.
    • Broadcast `PropIgnite {entityId,delayMs}` to sync delayed chain.
  hints:
    - delayMs random 100-300 using seed.



###############################################################################
# ███  PHASE VO — RADIO & SCANNER AUDIO  (VO-1)                              █
###############################################################################

- ticket_id: "VO-1"
  summary: "Centralised VO queue"
  context_files: []
  spec: >
    • `src/audio/VoiceOverQueue.reds` holds playEvents `{lineId,startTs}`.
    • Server authoritative; on trigger, sends `VOPlay {lineId}`.
    • Client plays Wwise event at same GameClock tick.
  hints:
    - Use pooled seeds to randomise ambient chatter.



###############################################################################
# ███  PHASE FX — FIXER PHONE-CALLS  (FX-1)                                  █
###############################################################################

- ticket_id: "FX-1"
  summary: "Shared fixer holo-calls"
  context_files: []
  spec: >
    • Hook `PhoneSystem.StartCall(fixerId)`.
    • Server broadcasts `FixerCallStart {fixerId}`; clients show subtitles and VO without UI lock.
    • On finish, send `FixerCallEnd`.
  hints:
    - Disable input freeze in multiplayer.



###############################################################################
# ███  PHASE GJ — GIG & SIDE-JOB SPAWN  (GJ-1)                               █
###############################################################################

- ticket_id: "GJ-1"
  summary: "Server-only trigger volumes"
  context_files: []
  spec: >
    • Convert open-world gig volumes to server-exclusive colliders.
    • When any player enters, server triggers `GigSpawn {questId,seed}`.
    • Clients instantiate quest NPCs/props from seed.
  hints:
    - Mark volume entities with tag "gigSrv".



###############################################################################
# ███  PHASE CD — CAR SUMMON / DELIVERY  (CD-1)                              █
###############################################################################

- ticket_id: "CD-1"
  summary: "Personal vehicle ownership lock"
  context_files: []
  spec: >
    • On summon request, server checks if vehicle already world-spawned.
      – If yes, teleports existing car to requester.
      – Else spawns new VehicleProxy ownerId=requester.
    • Broadcast `VehicleSummon {vehId,ownerId,pos}`.
  hints:
    - Deny summon if vehicle in combat and health<50%.



###############################################################################
# ███  PHASE WD — WARDROBE / TRANSMOG  (WD-1)                                █
###############################################################################

- ticket_id: "WD-1"
  summary: "AppearanceChange packet"
  context_files: []
  spec: >
    • `EMsg::Appearance {peerId, meshId, tintId}`.
    • Client AvatarProxy receives and reloads cloth mesh.
    • EquipCyberware reload also triggers Appearance pkt.
  hints:
    - Color tint uses TweakDB item tint.



###############################################################################
# ███  PHASE SP2 — SCAN & PING OVERLAY  (SPG-1)                              █
###############################################################################

- ticket_id: "SPG-1"
  summary: "Ping quickhack outline sharing"
  context_files: []
  spec: >
    • When player uses Ping, server gathers affected entityIds and sends
      `PingOutline {peerId, entityIds[], duration}`.
    • Clients draw outline shader for duration.
  hints:
    - Max 32 ids per packet.



###############################################################################
# ███  PHASE LT — LOOT RNG RE-ROLL CONTROL  (LT-1)                           █
###############################################################################

- ticket_id: "LT-1"
  summary: "Seeded loot container open"
  context_files: []
  spec: >
    • On container open, server generates `lootSeed` and sends `LootRoll {containerId,seed}`.
    • Client local RNG seeded before computing item drops.
  hints:
    - seed = FNV(time,containerId,playerId).



###############################################################################
# ███  PHASE AD — AUTO-DEALER VEHICLE PURCHASE  (AD-1)                       █
###############################################################################

- ticket_id: "AD-1"
  summary: "Dealer stock & purchase broadcast"
  context_files: []
  spec: >
    • Auto-dealer NPC exposes catalogue via `DealerStockSync`.
    • Purchase sends `DealerBuy {vehicleTpl,price}`; server verifies funds,
      sets ownership, broadcasts `VehicleUnlock {peerId, vehicleTpl}`.
    • WalletHud updates cost.
  hints:
    - Ownership list persisted via SaveFork.



###############################################################################
# ███  PHASE WI — WEAPON INSPECT & FINISHER  (WI-1 … WI-2)                   █
###############################################################################

- ticket_id: "WI-1"
  summary: "Inspect animation sync"
  context_files: []
  spec: >
    • Broadcast `WeaponInspectStart {peerId, animId}` when player plays inspect.
    • Other clients run same anim montage on proxy.
  hints:
    - Disable look input during inspect.

- ticket_id: "WI-2"
  summary: "Finisher cinematic sync"
  context_files: []
  spec: >
    • On melee finisher trigger, server sends `FinisherStart {actor,victim,animId}`.
    • Clients lock camera, play synced animation and sound.
    • Upon FinisherEnd, unlock cameras.
  hints:
    - Use CC-1 cinematic time-code for cam alignment.



###############################################################################
# ███  PHASE MB2 — GPU/VRAM BUDGET GUARD  (MB-3)                             █
###############################################################################

- ticket_id: "MB-3"
  summary: "Adaptive texture LOD down-scaler"
  context_files: []
  spec: >
    • Every 30 s, query `RenderDevice.GetVRAMUsage() / GetVRAMBudget()`.
    • If >90 % for >60 s, call `TextureSystem.SetGlobalMipBias(+1)`;
      log “MipBias +1”.
    • If <75 % for 2 min and bias>0, step bias −1.
    • Broadcast `TextureBiasChange` so all clients stay equal.
  hints:
    - Clamp mip bias 0-3.
```