# 🏃 Phase 2 — Player Replication Basics

```yaml
ticket_id: "P2-1"
summary: "Spawn & despawn RPCs"
context_files: []
spec: >
  Enable basic remote avatars:
    • Add `AvatarSpawn` and `AvatarDespawn` values to EMsg (Packets.hpp).
    • Create `src/runtime/AvatarProxy.reds`:
        class AvatarProxy extends gameObject {
          var peerId : Uint32;
          var isLocal : Bool;
        }
        func Spawn(peerId: Uint32, isLocal: Bool);
        func Despawn();
      For now, Spawn/Despawn only log actions.
    • Insert stub handlers in `Connection.cpp` that call these methods
      when the new messages arrive.
hints:
  - REDscript class only needs fields + log calls.
```
```yaml
ticket_id: "P2-2"
summary: "Transform snapshot sync"
context_files:
  - path: src/net/Snapshot.hpp
    excerpt: |
      // SnapshotFieldFlags placeholder
spec: >
  Add positional data to snapshots:
    • In `Snapshot.hpp`, add struct `TransformSnap { Vector3 pos; Vector3 vel; Quaternion rot; };`
    • Mark first three bits in `SnapshotFieldFlags` reserved for
      pos/vel/rot changes; comment indices.
    • Extend `AvatarProxy.reds` with fields `pos`, `vel`, `rot`
      and a `func ApplyTransform(snap: ref<TransformSnap>)`.
hints:
  - Use `Vector3` and `Quaternion` types available in REDscript stdlib.
```
```yaml
ticket_id: "P2-3"
summary: "Interpolation buffer"
context_files: []
spec: >
  Smooth remote motion:
    • Add `src/runtime/SnapshotInterpolator.reds` implementing:
        class SnapshotInterpolator {
          var buffer : array<TransformSnap>;
          func Push(tick: Uint64, snap: ref<TransformSnap>);
          func Sample(nowTick: Uint64, out snap: ref<TransformSnap>);
        }
      Inside `Sample`, return Hermite-interpolated transform
      between the two surrounding snapshots (stub math OK).
    • Update comments with formula reference.
hints:
  - No math engine yet—return linear lerp placeholder.
```
```yaml
ticket_id: "P2-4"
summary: "Client-side prediction & reconciliation"
context_files:
  - path: src/runtime/AvatarProxy.reds
    excerpt: |
      class AvatarProxy extends gameObject {
spec: >
  Add lightweight prediction hooks:
    • In `AvatarProxy.reds`, add:
        var pendingInputs : array<Uint16>; // placeholder
        func Predict(dt: Float);
        func Reconcile(authoritativeSnap: ref<TransformSnap>);
      Predict() just advances `pos += vel * dt` for now.
      Reconcile() snaps to authoritative position if error > 0.25 m.
    • Document in comments how pending inputs will be queued later.
hints:
  - No network plumbing yet—focus on method stubs & comments.
```
```yaml
ticket_id: "P2-5"
summary: "Health & armor bars"
context_files: []
spec: >
  Visualize player vitals:
    • Extend `TransformSnap` with `uint16 health; uint16 armor;`.
    • Under `/src/gui/` add `HealthBar.reds` that draws a simple
      floating bar above each `AvatarProxy` (stubbed draw call).
    • `AvatarProxy` now owns `var health : Uint16; var armor : Uint16`
      and updates the bar in a new `func OnVitalsChanged()`.
hints:
  - Bar rendering can be a comment placeholder; no ink widget code yet.
```
```yaml
ticket_id: "P2-6"
summary: "Ragdoll on disconnect & killfeed"
context_files:
  - path: src/runtime/AvatarProxy.reds
    excerpt: |
      func Despawn() {
spec: >
  Polish leave handling:
    • In `AvatarProxy.Despond()` swap the character to ragdoll via
      `SetRagdollMode(true)` then destroy after 5 s.
    • Add `/src/gui/Killfeed.reds` with:
        func Push(msg: String);
      Update Connection logic: when a peer leaves, call
      `Killfeed.Push("{peerId} disconnected")`.
hints:
  - Use simple log if SetRagdollMode is unavailable in stub.
```