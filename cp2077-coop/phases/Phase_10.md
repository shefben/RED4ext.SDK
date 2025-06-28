# Phase 10
```yaml
###############################################################################
# ███  NPC & CROWD  REPLICATION  (ticket IDs NR-1 … NR-4)                    █
###############################################################################
- ticket_id: "NR-1"
  summary: "NPC snapshot schema"
  context_files: []
  spec: >
    Lay the groundwork for replicating AI actors:
      • Add `EMsg::NpcSnapshot` and `EMsg::NpcSpawn` / `NpcDespawn`.
      • In `src/net/Snapshot.hpp` define struct `NpcSnap {
            Uint32 npcId;
            Uint16 templateId;     // archetype / crowd prefab
            Vector3 pos;
            Quaternion rot;
            Uint8  state;          // enum Idle, Wander, Combat…
            Uint16 health;
            Uint8  appearanceSeed;
        };`
      • Comment which fields are delta-tracked and which force full snap.
  hints:
    - Keep struct POD; align to 4 bytes.
    - health 0 → despawn event.

- ticket_id: "NR-2"
  summary: "Server-side NPC authority loop"
  context_files:
    - path: src/server/DedicatedMain.cpp
      excerpt: |
        // Net_Init already called
  spec: >
    Add deterministic server tick for NPCs:
      • Create `src/runtime/NpcController.reds`:
          func ServerTick(dt: Float);
          func ClientApplySnap(snap: ref<NpcSnap>);
      • Dedicated loop: every fixed tick call ServerTick and broadcast
        NpcSnapshot messages for changed NPCs.
  hints:
    - For now, AI logic = random walk with seeded RNG.

- ticket_id: "NR-3"
  summary: "Client NPC proxy & LOD"
  context_files:
    - path: src/runtime/NpcController.reds
      excerpt: |
        func ClientApplySnap(snap: ref<NpcSnap>);
  spec: >
    Visualise replicated NPCs:
      • Add `src/runtime/NpcProxy.reds` that spawns a minimal character
        mesh per `templateId`; updates transform/animState each snap.
      • Implement distance-based despawn (>120 m) on client only.
  hints:
    - Use placeholder mesh `"base\\characters\\crowd_man_01.mesh"`.

- ticket_id: "NR-4"
  summary: "Interest management for crowds"
  context_files: []
  spec: >
    Reduce bandwidth by streaming nearby NPCs only:
      • Maintain per-client `Set<Uint32> subscribedNpcs`.
      • Server includes NpcSnap only if
          distance(player, npc) < 80 m  OR npc.state == Combat.
      • Add comments on future spatial partition optimisation.
  hints:
    - No quadtree yet; simple distance loop acceptable for ≤300 NPCs.


###############################################################################
# ███  SECTOR  STREAMING / WORLD  CHUNKS  (SL-1 … SL-3)                      █
###############################################################################

- ticket_id: "SL-1"
  summary: "Sector identifier broadcast"
  context_files: []
  spec: >
    Start tracking player sectors:
      • Research `worldStreaming::SectorID` (doc comment).
      • Add `EMsg::SectorChange` with {peerId, sectorHash}.
      • Extend `AvatarProxy` field `currentSector : Uint64`.
      • Server sends SectorChange when player crosses load zone.
  hints:
    - sectorHash can be 64-bit FNV of sector name.

- ticket_id: "SL-2"
  summary: "Multi-sector interest list"
  context_files:
    - path: src/runtime/NpcController.reds
      excerpt: |
        // interest distance check
  spec: >
    Filter NPC snaps by sector:
      • Each NpcSnap now carries `Uint64 sectorHash`.
      • Server only includes NPCs whose sectorHash equals the
        client’s currentSector OR are flagged `alwaysRelevant`.
  hints:
    - alwaysRelevant for bosses, quest NPCs, etc.

- ticket_id: "SL-3"
  summary: "Asset-ready handshake"
  context_files: []
  spec: >
    Prevent soft-loads when teleporting far:
      • Add `EMsg::SectorReady`.
      • Client, after sector streaming complete (OnStreamingDone hook),
        sends SectorReady; server queues critical NPC/Quest snaps
        until this ack received.
  hints:
    - Log timeout if ack not received in 10 s.


###############################################################################
# ███  INVENTORY  &  CRAFTING  SYNC  (IC-1 … IC-3)                           █
###############################################################################

- ticket_id: "IC-1"
  summary: "Item stat replication"
  context_files: []
  spec: >
    Serialize full item state:
      • In `Snapshot.hpp` add `struct ItemSnap {
            Uint64 itemId; Uint32 ownerId;
            Uint16 tpl; Uint16 level; Uint16 quality;
            Uint32 rolls[4]; // random perks
        };`
      • Add `EMsg::ItemSnap` for delta updates.
  hints:
    - rolls[] deterministic via SeedPacket.

- ticket_id: "IC-2"
  summary: "Craft request / response flow"
  context_files: []
  spec: >
    Network crafting:
      • `EMsg::CraftRequest` (recipeId) client→server.
      • Server validates mats, consumes, then sends `CraftResult`
        with new ItemSnap.
      • Client adds item to inventory and plays success log.
  hints:
    - Validation placeholder: always succeeds if mats>0.

- ticket_id: "IC-3"
  summary: "Upgrade & attachment sync"
  context_files:
    - path: src/runtime/Inventory.reds
      excerpt: |
        // inventory table TBD
  spec: >
    Replicate weapon mod attachments:
      • Extend ItemSnap with `Uint8 slotMask; Uint64 attachmentIds[4];`
      • Implement AttachModRequest/Result packets.
      • Server rejects if slot already occupied.
  hints:
    - slotMask bit per attachment slot.


###############################################################################
# ███  VEHICLE  DESTRUCTION &  OCCUPANT  EJECTION  (VD-1 … VD-3)            █
###############################################################################

- ticket_id: "VD-1"
  summary: "Vehicle health & explosion sequence"
  context_files:
    - path: src/runtime/VehicleProxy.reds
      excerpt: |
        var damage : Uint16;
  spec: >
    Authoritative explosion flow:
      • When damage ≥1000 server broadcasts `VehicleExplode` packet.
      • Clients play explosion FX and mark vehicle destroyed.
      • Add cooldown before vehicle despawn (10 s).
  hints:
    - Use placeholder FX id `"veh_explode_small"`.

- ticket_id: "VD-2"
  summary: "Door detach & debris"
  context_files: []
  spec: >
    Sync detachable parts:
      • Add `EMsg::VehiclePartDetach {vehicleId, partId}`.
      • VehicleProxy.DetachPart() hides mesh `"door_L"` etc.
      • Trigger on side-collision over 300 dmg.
  hints:
    - partId enum comment: 0=door_L,1=door_R,2=hood,3=trunk.

- ticket_id: "VD-3"
  summary: "Occupant ejection ragdoll"
  context_files:
    - path: src/runtime/VehicleProxy.reds
      excerpt: |
        func ApplyDamage(d: Uint16);
  spec: >
    Eject passengers on high-g crash:
      • When decel >12 m/s², server sends `EjectOccupant {peerId}`.
      • AvatarProxy receives and calls `SetRagdollMode(true)`;
        applies 50 hp damage.
  hints:
    - Decel placeholder: delta vel / dt.


###############################################################################
# ███  HACKING MINI-GAME & BREACH PROTOCOL  (BH-1 … BH-3)                   █
###############################################################################

- ticket_id: "BH-1"
  summary: "Breach puzzle seed sync"
  context_files: []
  spec: >
    Start minigame:
      • `EMsg::BreachStart {peerId, seed, gridW, gridH}`.
      • Server picks RNG seed; clients build identical puzzle grid.
      • Create `src/gui/BreachHud.reds` that renders the grid (pseudo).
  hints:
    - Use seed to shuffle hex values deterministically.

- ticket_id: "BH-2"
  summary: "Shared timer & input broadcast"
  context_files:
    - path: src/gui/BreachHud.reds
      excerpt: |
        // grid render stub
  spec: >
    Sync player input:
      • While puzzle active, client broadcasts `BreachInput {idx}` when
        selecting a hex cell.
      • Server relays to all clients to highlight selection.
      • Shared timer counts down from 45 s; server authoritative.
  hints:
    - Highlight colour differs per peerId.

- ticket_id: "BH-3"
  summary: "Daemon effect application"
  context_files:
    - path: src/runtime/QuickhackSync.reds
      excerpt: |
        // HackInfo struct
  spec: >
    Apply Breach results:
      • After puzzle, server sends `BreachResult {peerId, daemonsMask}`.
      • Implement three daemons:
          bit0: Reduce enemy armour by 30 % (HeatSync hookup)
          bit1: Disable cameras for 90 s  (Future work comment)
          bit2: Mass vulnerability – +20 % dmg to all enemies
      • Log activation and duration timers.
  hints:
    - Use HeatSync to lower armour globally.


###############################################################################
# ███  INTERIOR TELEPORTS & ELEVATORS  (IE-1 … IE-3)                       █
###############################################################################

- ticket_id: "IE-1"
  summary: "Elevator interaction detection"
  context_files: []
  spec: >
    Send elevator usage packet:
      • Hook `workElevator::UseFloorButton()` (doc name) to detect
        when player selects a floor.
      • Broadcast `EMsg::ElevatorCall {peerId, elevatorId, floorIdx}`.
  hints:
    - elevatorId uint32 hash of entity name.

- ticket_id: "IE-2"
  summary: "Streaming-layer switch broadcast"
  context_files:
    - path: src/runtime/AvatarProxy.reds
      excerpt: |
        var currentSector : Uint64;
  spec: >
    When elevator arrives:
      • Server sends `ElevatorArrive {elevatorId, sectorHash, pos}`.
      • Clients update currentSector and teleport to pos after sector ready.
  hints:
    - reuse SectorReady handshake from SL-3.

- ticket_id: "IE-3"
  summary: "Teleport soft-lock safeguard"
  context_files: []
  spec: >
    Ensure peers don’t desync after teleport:
      • After ElevatorArrive, clients send TeleportAck.
      • If any peer fails to ack within 8 s, server pauses world state
        broadcasts and re-sends ElevatorArrive.
  hints:
    - Add retry counter, max 3 attempts.


###############################################################################
# ███  NETWORK-SAFE UI  CLEAN-UP  (UI-1 … UI-3)                             █
###############################################################################

- ticket_id: "UI-1"
  summary: "Blocking UI audit & blocklist"
  context_files: []
  spec: >
    Identify world-pausing UIs:
      • Create `src/runtime/UIPauseAudit.reds` that hooks
        `inkMenuLayer::OnOpen`.
      • Log menu name; if it pauses world and not whitelisted, add to
        `blockedMenus` array and print warning.
      • Seed array with known offenders: "WorldMap", "Journal", "Shards".
  hints:
    - No blocking yet—just audit.

- ticket_id: "UI-2"
  summary: "Replace world map with synced overlay"
  context_files:
    - path: src/runtime/UIPauseAudit.reds
      excerpt: |
        const blockedMenus : array<CName>
  spec: >
    Provide co-op safe map:
      • Intercept opening of "WorldMap"; instead open new
        `src/gui/CoopMap.reds` overlay that shows players’ icons only.
      • CoopMap can pan/zoom but does NOT pause the game.
  hints:
    - Use existing minimap textures for placeholder.

- ticket_id: "UI-3"
  summary: "Shard reading & holo-call handling"
  context_files: []
  spec: >
    Final UI desync fixes:
      • Detect when player opens shard reading (braindance viewer);
        if in co-op, show CoopNotice instead.
      • For holo-calls, ensure cinematic letterbox does not freeze other
        peers: broadcast `HoloCallStart/End` packets to hide/show overlay.
  hints:
    - Hook `phonePhoneSystem::StartCall`.
```