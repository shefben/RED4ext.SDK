# Phase 17 — Phased Experience & Buyable Apartments

```yaml
###############################################################################
# ███  PHASE PX — OPEN-WORLD “PHASED EXPERIENCE” (PX-1 … PX-12)             █
# Fully isolates quest/NPC instances per player while sharing Night City.   █
###############################################################################

- ticket_id: "PX-1"
  summary: "Add phaseId to core structs"
  context_files: []
  spec: >
    • In `src/net/Snapshot.hpp` append `uint32_t phaseId` to:
        – NpcSnap
        – AvatarSpawnPkt / Despawn
        – SceneTriggerPkt
    • 0 = global world, peerId = personal phase, 0xFF_FF… reserved for debug.
  hints:
    - Adjust sizeof checks and static_asserts.

- ticket_id: "PX-2"
  summary: "QuestStageP2P packet"
  context_files:
    - path: src/runtime/QuestSync.reds
      excerpt: |
        // QuestStagePkt legacy
  spec: >
    • Deprecate `QuestStagePkt`; add `QuestStageP2P {phaseId,questHash,stage}`.
    • Server stores per-phase quest map `Dict<phaseId,Dict<quest,stage>>`.
    • ApplyQuestStage now indexes by localPhase.
  hints:
    - phaseId == peerId for personal stories.

- ticket_id: "PX-3"
  summary: "SnapshotWriter phase filter"
  context_files:
    - path: src/net/SnapshotWriter.cpp
      excerpt: |
        // iterate entities
  spec: >
    Skip entity if `snap.phaseId != localPhase`
    unless `spectatePhase == snap.phaseId`.
  hints:
    - spectatePhase defaults to localPhase.

- ticket_id: "PX-4"
  summary: "Solo cinematic flag"
  context_files: []
  spec: >
    • Extend CineStartPkt with `bool solo`.
    • If solo && packet.phaseId != localPhase → ignore camera freeze.
    • Otherwise behave as existing group scene.
  hints:
    - Add HUD watermark “Teammate cinematic” for spectators.

- ticket_id: "PX-5"
  summary: "Phase trigger duplication API"
  context_files: []
  spec: >
    • `TriggerSystem.SpawnPhaseTrigger(baseEntId, phaseId)`.
    • On quest step start, server clones trigger entity to that phase.
    • Auto-destroy when quest completes or soft-lock watchdog resyncs.
  hints:
    - Use naming suffix “_ph<phaseId>”.

- ticket_id: "PX-6"
  summary: "Critical-quest convergence vote"
  context_files: []
  spec: >
    • New JSON `CriticalQuests.json` listing {questHash,stage}.
    • When any phase reaches critical record, server issues
      `CriticalVoteStart {questHash}` to all peers.
    • Majority “Yes” within 30 s forces all phases to sync to host stage.
  hints:
    - Default auto-yes if peers AFK (no input within timer).

- ticket_id: "PX-7"
  summary: "Hot-Join phase bundle"
  context_files: []
  spec: >
    • Late-join snapshot now includes `PhaseBundle {phaseId, QuestMap, NpcList}`.
    • Client applies bundles: localPhase first; others cached for spectators.
  hints:
    - Compress bundle via zstd.

- ticket_id: "PX-8"
  summary: "Vendor per-phase inventory"
  context_files: []
  spec: >
    • VendorController.inventory becomes `Dict<phaseId,Dict<itemTpl,qty>>`.
    • If vendor JSON has `"shareStock":true`, use phaseId 0 (global).
  hints:
    - Update EC-3 stock sync to include phaseId.

- ticket_id: "PX-9"
  summary: "Quest HUD phase filter"
  context_files: []
  spec: >
    • QuestTracker.reds draws objectives only where `phaseId==localPhase`.
    • Add `/assist <peer>` command to switch `localPhase` temporarily.
  hints:
    - Display small banner “Assisting <name>”.

- ticket_id: "PX-10"
  summary: "Assist / spectate toggle logic"
  context_files:
    - path: src/runtime/SpectatorCam.reds
      excerpt: |
        var spectatePhase : Uint32 = localPhase
  spec: >
    • `/assist off` reverts `spectatePhase=localPhase`.
    • Snapshot filter uses `spectatePhase`.
  hints:
    - Don’t allow assist while in combat.

- ticket_id: "PX-11"
  summary: "Phase memory GC"
  context_files: []
  spec: >
    • Server tracks `lastActiveTick` per phase.
    • If inactive >10 min and no players in phase:
        – despawn NPCs, triggers
        – clear snapshot baselines
    • Log cleanup summary.
  hints:
    - Don’t GC phaseId 0 (global).

- ticket_id: "PX-12"
  summary: "SaveFork per-phase files"
  context_files:
    - path: src/core/SaveFork.cpp
      excerpt: |
        // single session json
  spec: >
    • Write `phase_<peerId>.json.zst` inside session dir.
    • coop_merge merges ONLY caller’s phase file.
  hints:
    - Maintain index file listing owned phaseIds.


###############################################################################
# ███  PHASE AP — BUYABLE APARTMENTS (AP-1 … AP-5)                           █
###############################################################################

- ticket_id: "AP-1"
  summary: "Apartment data model & packets"
  context_files: []
  spec: >
    • New CSV `Apartments.csv` with ids, coords, price, interiorScene, extDoorId.
    • Packets:
        – `AptPurchase {aptId}`
        – `AptPurchaseAck {aptId,success,balance}`
        – `AptEnterReq {aptId,ownerPhaseId}`
        – `AptEnterAck {allow, phaseId, interiorSeed}`
        – `AptPermChange {aptId,targetPeerId,allow}`
  hints:
    - ownerPhaseId == ownerPeerId.

- ticket_id: "AP-2"
  summary: "EZ-Estates UI integration"
  context_files: []
  spec: >
    • On V’s computer, replace vanilla EZ-Estates webpage with
      Coop variant:
        – Shows all apartments (price, “Owned” tag).
        – If owned, list permitted peers, Add/Remove buttons.
        – Buy button sends AptPurchase pkt.
  hints:
    - Use same HTML/CSS container; replace JS via WebView API.

- ticket_id: "AP-3"
  summary: "Server-side purchase flow & wallet deduction"
  context_files:
    - path: src/runtime/Inventory/Vendor.reds
      excerpt: |
        // wallet functions
  spec: >
    • Server validates player balance ≥ price.
    • On success: deduct, create entry `ApartmentOwnership {aptId,owner=peerId}`.
    • Broadcast AptPurchaseAck to buyer only.
    • Update WalletHud.
  hints:
    - Ownership stored in SaveFork.phase_<peerId>.

- ticket_id: "AP-4"
  summary: "Interior instance streaming & teleport"
  context_files: []
  spec: >
    • On AptEnterReq, host loads interiorScene if not loaded for ownerPhase.
    • Server teleports requesting peer to doorSpawn inside that phase.
    • Interior objects (stash, mirrors) carry `phaseId = ownerPeerId`.
    • Upon exit, peer returns to exterior door coord.
  hints:
    - Use existing SectorReady handshake to avoid world-hole.

- ticket_id: "AP-5"
  summary: "Permission UI & packet handling"
  context_files:
    - path: src/gui/EZEstate.reds
      excerpt: |
        // Add/Remove peer UI
  spec: >
    • Owner can whitelist peers by SteamID or choose “public”.
    • AptPermChange broadcast updates server dict `aptPerms[aptId]`.
    • When non-owner AptEnterReq, server checks perms; denies or clones interior phase if allowed.
  hints:
    - Deny enters while owner in quest-locked cinematic.


###############################################################################
# ★  RUN ORDER RECOMMENDATION
#   1. PX-1 → PX-4 (core phase plumbing)  
#   2. PX-5 → PX-8 (triggers, critical quests, vendors)  
#   3. PX-9 → PX-12 (HUD, assist, persistence)  
#   4. AP-1 … AP-5 (apartments)  
###############################################################################
```