# Phase 9

```yaml
ticket_id: "R1-1"
summary: "Full NetCore networking loop"
context_files:
  - path: src/net/Net.cpp
    excerpt: |
      // Net_Init / Net_Shutdown stubs
spec: >
  Replace Net stubs with a functional ENet wrapper:
    • Initialize enet, create host (max 8 peers) in Net_Init().
    • In Net_Shutdown(), destroy host and deinit enet.
    • Add Net_Poll(uint32_t maxMs) that:
        while (enet_host_service(host,&event,maxMs)>0) {
          dispatch event to Connection::HandlePacket() or
          create/destroy Connection objects.
        }
    • Update CMakeLists.txt: link `enet` for both mod and dedicated targets.
hints:
  - Use non-blocking host `enet_host_service`.
  - No actual game packets yet—just forward raw PacketHeader+payload to Connection.
```
```yaml
ticket_id: "R1-2"
summary: "Implement SnapshotWriter/Reader"
context_files:
  - path: src/net/Snapshot.hpp
    excerpt: |
      class SnapshotWriter { /* Begin(), Write<T>(), End() */ };
spec: >
  Fill in SnapshotWriter/Reader:
    • SnapshotWriter:
        Begin(id,baseId);
        template<TriviallyCopyable T> void Write(fieldBit,T value);
        size_t End(uint8_t* outBuf,size_t max);
      Tracks dirty bits; writes header + diff payload.
    • SnapshotReader:
        Attach(buf); bool Has(fieldBit); template<T> T Read();
      Stores baseline id for delta chain.
    • Unit-test static fixture in tests/static/snapshot_roundtrip.json
      (add JSON with known values and expected bytes).
hints:
  - Use `memcpy` for trivially-copyable types; avoid templates in REDscript.
```
```yaml
ticket_id: "R1-3"
summary: "Hermite interpolation in SnapshotInterpolator"
context_files:
  - path: src/runtime/SnapshotInterpolator.reds
    excerpt: |
      func Sample(nowTick: Uint64, out snap: ref<TransformSnap>) {
spec: >
  Replace linear lerp with Hermite:
    • Implement Catmull-Rom style Hermite interpolation for pos & rot.
    • Require at least 2 snapshots before / after; fall back to linear if unavailable.
    • Update comments with formula and citation.
hints:
  - Use REDscript’s `QuatSlerp` for rotation.
```
```yaml
ticket_id: "R1-4"
summary: "Complete client prediction system"
context_files:
  - path: src/runtime/AvatarProxy.reds
    excerpt: |
      var pendingInputs : array<Uint16>;
spec: >
  Implement real input buffering:
    • Define struct `MoveCmd { Uint16 seq; Vector3 move; Quaternion rot; };`
    • Collect MoveCmd each frame, push to pendingInputs.
    • When authoritative Snapshot arrives, reconcile:
        – drop cmds ≤ snapshot.seq
        – re-simulate remaining cmds
        – log correction distance.
    • Clamp correction teleport if >1.5 m.
hints:
  - Keep arrays ≤ 30 cmds to bound memory.
```
```yaml
ticket_id: "R1-5"
summary: "Hook QuestSystem::AdvanceStage"
context_files:
  - path: src/runtime/QuestSync.reds
    excerpt: |
      func OnAdvanceStage(questName: CName);
spec: >
  Wire up Quest stage sync:
    • Use RED4ext’s `hook` attribute to detour `QuestSystem::AdvanceStage`.
    • Server-side: call original, then SendQuestStageMsg().
    • Client-side: ApplyQuestStage() sets queststage via QuestSystem API.
    • Add debug log: "[QuestSync] {questName} stage advanced".
hints:
  - Include `#include <RED4ext/Scripting/Natives/Generated/questQuestSystem.hpp>`
```
```yaml
ticket_id: "R1-6"
summary: "Replace blocked pop-ups with coop notice"
context_files:
  - path: src/runtime/PopupGuard.reds
    excerpt: |
      func ReplaceWithCoopNotice(id: CName);
spec: >
  Implement ink widget notice:
    • Build `CoopNotice.reds` inkLayer that shows center-screen text
      "{popupName} disabled in co-op".
    • ReplaceWithCoopNotice() should instantiate the layer for 3 s.
    • Maintain a static singleton so notices do not stack.
hints:
  - Use inkText with style "bold 36px".
```
```yaml
ticket_id: "R1-7"
summary: "Finalize ServerBrowser UI"
context_files:
  - path: src/gui/ServerBrowser.reds
    excerpt: |
      func Refresh(jsonList: String);
spec: >
  Replace placeholder with ink list:
    • Build an `inkListController` displaying rows (server name, player count).
    • OnRowClicked() highlights selection and enables Join button.
    • Fetch sample list from `tests/static/serverlist.json`.
    • Scroll wheel navigates list; arrow keys move selection.
hints:
  - Use `inkScrollController` for list movement.
```
```yaml
ticket_id: "R1-8"
summary: "Integrate CarPhysics into VehicleProxy"
context_files:
  - path: src/physics/CarPhysics.cpp
    excerpt: |
      void ServerSimulate(TransformSnap& inout, float dtMs);
spec: >
  Connect VehicleProxy to deterministic physics:
    • VehicleProxy.Tick(dtMs): 
        if Net_IsAuthoritative() -> ServerSimulate()
        else                     -> ClientPredict()
    • After simulation, package TransformSnap and broadcast.
    • ClientPredict() uses last authoritative vel+accel.
hints:
  - Ensure same `kFixedDeltaMs` as GameClock.
```
```yaml
ticket_id: "R1-9"
summary: "Apply quickhack gameplay effects"
context_files:
  - path: src/runtime/QuickhackSync.reds
    excerpt: |
      func ApplyHack(info: ref<HackInfo>);
spec: >
  Implement basic quickhack logic:
    • If hackId == 1 ("ShortCircuit"), reduce target health by 25%
      over duration; tick every 500 ms.
    • Maintain activeHacks : array<HackInfo>.
    • Remove hack when duration expires.
hints:
  - Use `GameInstance.GetPlayerSystem().FindObject(targetId)` to
    resolve target.
```
```yaml
ticket_id: "R1-10"
summary: "Finish respawn flow & scoreboard updates"
context_files:
  - path: src/runtime/Respawn.reds
    excerpt: |
      func PerformRespawn(peerId: Uint32);
spec: >
  Finalize DM flow:
    • Randomly choose spawn point, teleport AvatarProxy.
    • Reset health/armor to max, zero velocity.
    • Update DMScoreboard.kills/deaths and broadcast ScoreUpdate packet.
    • Scoreboard listens to ScoreUpdate and updates HUD.
hints:
  - Spawn point index uses `rand() % spawnPoints.Size()`.
```
```yaml
ticket_id: "R1-11"
summary: "Replace raw arrays with ThreadSafeQueue"
context_files:
  - path: src/core/ThreadSafeQueue.hpp
    excerpt: |
      template<typename T> class ThreadSafeQueue {
spec: >
  Use ThreadSafeQueue for inbound packet buffering:
    • Add `ThreadSafeQueue<RawPacket>` member to Connection.
    • Net_Poll pushes RawPacket into queue; Game thread pops.
    • Document potential deadlock scenarios avoided.
hints:
  - RawPacket = struct { PacketHeader hdr; std::vector<uint8_t> data; }.
```
```yaml
ticket_id: "R1-12"
summary: "Implement SaveFork path creation"
context_files:
  - path: src/core/SaveFork.cpp
    excerpt: |
      void EnsureCoopSaveDirs();
spec: >
  Add real filesystem calls:
    • Use `<filesystem>` to create directories if missing.
    • Implement SaveSession(jsonBlob) that writes
      `{sessionId}.json` under kCoopSavePath.
    • Log success/failure with path.
hints:
  - Assume UTF-8 paths; wrap I/O in try/catch.
```