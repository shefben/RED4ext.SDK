# 🚗 Phase 6 — Vehicles & Mounts

```yaml
ticket_id: "P6-1"
summary: "Vehicle spawn replication"
context_files: []
spec: >
  Spawn vehicles over network:
    • Add `VehicleSpawn` to EMsg.
    • `src/runtime/VehicleProxy.reds`:
        var vehicleId : Uint32;
        func Spawn(id: Uint32, transform: ref<TransformSnap>);
    • Server assigns id & transform; clients log spawn.
hints:
  - Use same TransformSnap struct.
```
```yaml
ticket_id: "P6-2"
summary: "Seat assignment RPC"
context_files:
  - path: src/runtime/VehicleProxy.reds
    excerpt: |
      class VehicleProxy extends gameObject {
spec: >
  Network seat entry:
    • Add `SeatAssign` to EMsg with fields {peerId, vehicleId, seatIdx}.
    • In `VehicleProxy.reds`, add func EnterSeat(peerId: Uint32, idx: Uint8);
      which logs "{peerId} entered seat {idx}".
hints:
  - SeatIdx range comment 0-3.
```
```yaml
ticket_id: "P6-3"
summary: "Deterministic car physics"
context_files: []
spec: >
  Authoritative vehicle simulation:
    • Add `src/physics/CarPhysics.cpp` + `.hpp`:
        void ServerSimulate(TransformSnap& inout, float dtMs);
        void ClientPredict(TransformSnap& inout, float dtMs);
      Provide fixed-step Euler integration placeholder.
    • Comment on matching server vs client drift tolerance.
hints:
  - Maths only; call sites later.
```
```yaml
ticket_id: "P6-4"
summary: "Collision damage sync"
context_files: []
spec: >
  Share vehicle crash damage:
    • Append `VehicleHit` to EMsg with fields {vehicleId, dmg}.
    • `src/runtime/VehicleProxy.reds` add func ApplyDamage(d: Uint16);
      that logs current cumulative damage.
hints:
  - Damage calc will use CarPhysics later.
```
