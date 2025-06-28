# 📦 Phase 1 — Core Session Pipeline

```yaml
ticket_id: "P1-1"
summary: "Packet enums & header"
context_files: []
spec: >
  Create `src/net/Packets.hpp` defining the low-level wire format:
    • `enum class EMsg : uint16_t` with at least:
        Hello=1, Welcome, Ping, Pong, Seed, Snapshot, Chat,
        JoinRequest, JoinAccept, JoinDeny, Disconnect.
    • `struct PacketHeader { uint16_t type; uint16_t size; };`
      plus `constexpr size_t kHeaderSize = sizeof(PacketHeader);`
    • Inline helpers:
        constexpr size_t Packet_GetSize(const PacketHeader&);
        void Packet_SetSize(PacketHeader&, uint16_t payloadBytes);
    • `static_assert(sizeof(PacketHeader) == 4, "header must be packed");`
    • Header-only; no cpp file yet.  Add a top-of-file comment showing
      an example JSON representation of a header `{type:1,size:42}`.
hints:
  - Use `<cstdint>` and `constexpr` only; no external deps.
```
```yaml
ticket_id: "P1-2"
summary: "Connection state machine"
context_files: []
spec: >
  Implement a minimal connection state helper:
    • Add `src/net/Connection.hpp` and `.cpp`.
    • `enum class ConnectionState { Disconnected, Handshaking,
        Lobby, InGame };`
    • Class `Connection` members:
        state          : ConnectionState
        lastPingSent   : uint64_t   // ms
        lastRecvTime   : uint64_t   // ms
      Key methods (logic only, no sockets yet):
        void StartHandshake();
        void HandlePacket(const PacketHeader& hdr);
        void Update(uint64_t nowMs);
      Log every state transition via RED4ext logger stub.
hints:
  - Just store timestamps; time source comes later via GameClock.
```
```yaml
ticket_id: "P1-3"
summary: "Authoritative host flag"
context_files: []
spec: >
  Add `src/net/NetConfig.hpp` containing:
    constexpr bool kDedicatedAuthority = true;
  Include a short comment explaining we will consider host migration
  later but all logic currently assumes server authority.
hints:
  - Header-only; no cpp needed.
```
```yaml
ticket_id: "P1-4"
summary: "Fixed tick-lock clock"
context_files: []
spec: >
  Introduce a deterministic simulation clock:
    • Files `src/core/GameClock.hpp` and `.cpp`.
    • `constexpr float kFixedDeltaMs = 32.f;`
    • Class `GameClock` static interface:
        void Tick(float dtMs);          // accumulates elapsed time
        uint64_t GetCurrentTick();      // returns tick index
        float    GetTickAlpha(float nowMs); // 0–1 interpolation alpha
      Document usage in header comments.
hints:
  - No real loop yet—just pure math + static variables.
```
```yaml
ticket_id: "P1-5"
summary: "RNG seed sync packet"
context_files:
  - path: src/net/Packets.hpp
    excerpt: |
      enum class EMsg : uint16_t {
          Hello = 1,
          Welcome,
          ...
spec: >
  Extend networking for deterministic RNG:
    • Append `Seed`/`SeedAck` to `EMsg`.
    • Add `struct SeedPacket { uint32_t seed; };` in `Packets.hpp`.
    • Comment an example exchange:
        client → server : SeedRequest
        server → all    : Seed(seed=123456u)
    • No runtime logic yet—just the packet definition.
hints:
  - Keep structs POD and pack-safe.
```
```yaml
ticket_id: "P1-6"
summary: "Delta-snapshot writer/reader skeleton"
context_files: []
spec: >
  Lay out snapshot infrastructure:
    • New file `src/net/Snapshot.hpp` with:
        using SnapshotId = uint32_t;
        struct SnapshotFieldFlags { uint32_t bits[4]; }; // 128 flags
        struct SnapshotHeader { SnapshotId id; SnapshotId baseId; };
    • Class stubs:
        class SnapshotWriter { /* Begin(), Write<T>(), End() */ };
        class SnapshotReader { /* Attach(buffer), Read<T>() */ };
    • Provide detailed comments on dirty-bit tracking, baseline
      chaining, and delta compression—no implementation yet.
hints:
  - Forward declarations only; real code arrives in later tickets.
```