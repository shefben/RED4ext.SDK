# 🌍 Phase 4 — World & Quest Sync

```yaml
ticket_id: "P4-1"
summary: "Quest stage broadcast"
context_files: []
spec: >
  Synchronize quest progress:
    • Add `QuestStage` value to `EMsg` in `src/net/Packets.hpp`.
    • Create `src/runtime/QuestSync.reds` with:
        func OnAdvanceStage(questName: CName);
        func SendQuestStageMsg(questName: CName);
        func ApplyQuestStage(questName: CName);
      OnAdvanceStage (server-side) should call SendQuestStageMsg().
      Clients call ApplyQuestStage() when the packet arrives (log only).
    • Update comments explaining why quest desync soft-locks players.
hints:
  - Hook `QuestSystem::AdvanceStage(questName)` via RED4ext RTTI.
````
```yaml
ticket_id: "P4-2"
summary: "Trigger / scene sync"
context_files:
  - path: src/runtime/QuestSync.reds
    excerpt: |
      // Quest stage sync stubs
spec: >
  Mirror cutscene and scene triggers:
    • Add `SceneTrigger` to `EMsg`.
    • In `QuestSync.reds` add:
        func OnSceneStart(id: TweakDBID);
        func OnSceneEnd(id: TweakDBID);
      Both functions should broadcast SceneTrigger packets that
      carry `id` and a `bool isStart`.
    • Client handler starts/stops local scene (log placeholder).
hints:
  - Use TweakDBID to identify scenes; no animation logic yet.
````
```yaml
ticket_id: "P4-3"
summary: "Inventory authority"
context_files: []
spec: >
  Prevent duplicated loot:
    • Extend `TransformSnap` (Snapshot.hpp) with `uint32 ownerId`.
    • Create `src/runtime/LootAuthority.reds`:
        func MarkOwnership(itemId: Uint64, ownerId: Uint32);
        func CanPickup(itemId: Uint64, peerId: Uint32) -> Bool;
    • On item drop, server sets ownerId = 0 (public).
      Clients call CanPickup() before allowing pickup (log only).
hints:
  - Ownership logic purely table-based; no UI updates yet.
````
```yaml
ticket_id: "P4-4"
summary: "Net-safe pop-ups"
context_files: []
spec: >
  Disable single-player-only pop-ups:
    • Add `src/runtime/PopupGuard.reds` with:
        func IsPopupBlocked(id: CName) -> Bool;
        func ReplaceWithCoopNotice(id: CName);
    • Hook sleep/braindance pop-ups; if IsPopupBlocked returns true,
      skip the original UI and call ReplaceWithCoopNotice which logs
      "{id} blocked for co-op".
hints:
  - Maintain a static array of blocked IDs for now.
````
```yaml
-ticket_id: "P4-5"
summary: "Savegame forking"
context_files: []
spec: >
  Separate co-op save files:
    • Add `src/core/SaveFork.cpp` + `.hpp`.
    • Constant `kCoopSavePath = "SavedGames/Coop/"`.
    • Provide:
        std::string GetSessionSavePath(sessionId);
        void EnsureCoopSaveDirs();
      All functions just build strings and log the path.
hints:
  - No actual I/O yet; focus on path logic.
```