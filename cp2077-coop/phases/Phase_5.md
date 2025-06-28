# 🔫 Phase 5 — Combat Fidelity

```yaml
ticket_id: "P5-1"
summary: "Hitscan lag compensation"
context_files: []
spec: >
  Add server-side rewind math:
    • `src/physics/LagComp.hpp` and `.cpp`:
        Vector3 RewindPosition(const Vector3& pos, const Vector3& vel,
                               float clientRTTms);
    • Comment formula: rewind = pos - vel * (clientRTTms / 1000.f).
    • Extend Packets with `HitRequest` & `HitConfirm`.
hints:
  - Only math & packet structs; no networking glue yet.
```
```yaml
ticket_id: "P5-2"
summary: "Melee rollback buffer"
context_files: []
spec: >
  Buffer recent snapshots for melee checks:
    • In `SnapshotInterpolator.reds`, add array `history : array<TransformSnap>;`
      size 10 (stores last ~320 ms).
    • func GetSnapshotAt(tick: Uint64) -> ref<TransformSnap>;
      returns closest snap (linear search).
hints:
  - Pure REDscript; snapshot push already implemented in P2-3.
```
```yaml
ticket_id: "P5-3"
summary: "Quickhack sync"
context_files: []
spec: >
  Replicate quickhack effects:
    • Add `Quickhack` to EMsg.
    • Create `src/runtime/QuickhackSync.reds` with:
        struct HackInfo { Uint32 targetId; Uint32 hackId; Uint16 durationMs; };
        func SendHack(info: ref<HackInfo>);
        func ApplyHack(info: ref<HackInfo>);
      Server validates then broadcasts; clients apply (log only).
hints:
  - No effect visuals yet; focus on data path.
```
```yaml
ticket_id: "P5-4"
summary: "Armor damage validation"
context_files: []
spec: >
  Prevent cheater one-shots:
    • Add `src/server/DamageValidator.cpp`:
        bool ValidateDamage(uint16 rawDmg, uint16 targetArmor);
      Rule: damage cannot exceed (armor * 4 + 200).
    • Log "cheat detected" if validation fails.
hints:
  - Static function only; integration later.
```
```yaml
ticket_id: "P5-5"
summary: "Shared NCPD heat"
context_files: []
spec: >
  Sync wanted level:
    • Add `HeatSync` to EMsg.
    • `src/runtime/HeatSync.reds`:
        var heatLevel : Uint8;
        func BroadcastHeat(level: Uint8);
        func ApplyHeat(level: Uint8);
      Broadcast every time level changes.
hints:
  - Use enum 0–5 for star levels.
```
```yaml
ticket_id: "P5-6"
summary: "World state sync (time & weather)"
context_files: []
spec: >
  Keep atmosphere identical for all:
    • Add `WorldState` to EMsg with payload:
        UInt32 sunAngle;   // degrees * 100
        UInt8  weatherId;  // simple enum
    • Create `src/runtime/WeatherSync.reds` broadcasting every 30 s
      or immediately on change (server).
    • Clients apply by logging state change.
hints:
  - No actual graphic hooks yet.
```