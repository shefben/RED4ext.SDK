# Phase 18

```yaml
###############################################################################
# ███  PHASE CP — CAR PERSISTENCE & PARKING  (CP-1)                          █
###############################################################################

- ticket_id: "CP-1"
  summary: "Persist last parked position per owner"
  context_files:
    - path: src/core/SaveFork.cpp
      excerpt: |
        // phase_<peerId>.json.zst structure
  spec: >
    Extend SaveFork JSON schema:
      • Add optional `CarParking` object:
          {
            "vehTpl"   : uint32,   // TweakDB ID of vehicle template
            "pos"      : [x,y,z],
            "rot"      : [qx,qy,qz,qw],
            "health"   : uint16
          }
      • On `VehicleSummon` **despawn** (player leaves car >2 m & vehicle
        idle 10 s), server records CarParking under owner’s phase file.
      • On next session load or Hot-Join, if CarParking exists, spawn the
        car at that transform *instead of* default summon point.
  hints:
    - Use existing `TransformSnap` serialization helpers.
    - Clear CarParking on explosion (health == 0).

###############################################################################
# ███  PHASE VC — VEHICLE COLLISION LATENCY COMP  (VC-1)                     █
###############################################################################

- ticket_id: "VC-1"
  summary: "50 ms rewind for high-speed vehicle collisions"
  context_files:
    - path: src/physics/CarPhysics.cpp
      excerpt: |
        // ServerSimulate() collision handling
  spec: >
    Improve hit-registration fairness between two player-owned cars:
      • On collision detection, compute `impactSpeed` for both cars.
      • If `impactSpeed >= 200 km/h`, rewind both cars by
        `rewindDt = min(latencyA, latencyB, 50 ms)` using their
        stored velocity, recompute collision, then apply final delta.
      • Broadcast `VehicleHitHighSpeed {vehA,vehB,deltaVel}` for
        deterministic replay on clients.
  hints:
    - Use latency from Connection.pingMs; convert to dt.

###############################################################################
# ███  PHASE TR — TOW / REPAIR FLOW  (TR-1)                                  █
###############################################################################

- ticket_id: "TR-1"
  summary: "/unstuckcar command & wreck respawn timer"
  context_files:
    - path: src/server/DedicatedMain.cpp
      excerpt: |
        // command parsing loop
  spec: >
    Quality-of-life for destroyed or lost cars:
      1. **Auto-return**  
         • After a VehicleExplode despawn, start 5-min timer.  
         • When timer fires, mark the vehicle “available” in
           owner’s summon pool; log “[Tow] Car returned”.
      2. **Manual /unstuckcar**  
         • New chat or console command `/unstuckcar`.  
         • Immediately teleports owned vehicle to a safe coord
           near player (if alive) **or** flags summon available if wrecked.
      • Both flows send `VehicleTowAck {ownerId,ok}`.
  hints:
    - Safe coord: `NavSystem.FindNearestRoadNode(playerPos)`.

###############################################################################
# ███  PHASE VS — PER-PHASE VEHICLE SPAWNS  (VS-1)                           █
###############################################################################

- ticket_id: "VS-1"
  summary: "Phase-aware vehicle spawner"
  context_files:
    - path: src/runtime/VehicleProxy.reds
      excerpt: |
        var phaseId : Uint32 = 0   // default global
  spec: >
    Allow quest-specific or instanced cars:
      • Modify `VehicleSpawnPkt` to include `phaseId`.
      • `VehicleProxy.phaseId` set from packet.
      • SnapshotWriter filters vehicles the same way as NPCs:
          send if `veh.phaseId == interestPhase || spectate`.
      • Provide helper `SpawnPhaseVehicle(vehTpl, transform, phaseId)`.
      • Update CD-1 personal summon to use `phaseId = 0` (global) as before.
  hints:
    - For PX quests, pass `phaseId = ownerPeerId` to make cars private.
```